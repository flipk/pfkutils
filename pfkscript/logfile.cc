/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include "options.h"
#include "logfile.h"

#include <sys/types.h>
#include <glob.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>

#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <algorithm>

#include "pfkpthread.h"

using namespace std;

void
printError(const std::string &func)
{
    int e = errno;

#if 1
    cerr << func << ": " << e << ": " << strerror(e) << endl;
#else

// fuck this, strerror_r sucks and i know i'm not doing this in more
// than one thread. see:
// http://stackoverflow.com/questions/3051204/strerror-r-returns-trash-when-i-manually-set-errno-during-testing

    char errbuf[128];
    errbuf[0] = 0;
    strerror_r(e, errbuf, sizeof(errbuf));

    cerr << func << ": " << e << ": " << errbuf << endl;

#endif
}

LogFile :: LogFile(const Options &_opts)
    : opts(_opts), isError(true), counter(0), currentStream(NULL)
{
    size_t slashPos = opts.logfileBase.find_last_of('/');
    if (slashPos == string::npos)
    {
        logDir = ".";
        logFilebase = opts.logfileBase;
    }
    else
    {
        logDir = opts.logfileBase.substr(0,slashPos);
        logFilebase = opts.logfileBase.substr(slashPos+1);
    }

    if (0) // debug
    {
        cout << "logdir = " << logDir << endl;
        cout << "logFilebase = " << logFilebase << endl;

        LfeList list;
        globLogFiles(list);

        cout << "glob matches:\n";
        for (int ind = 0; ind < list.size(); ind++)
        {
            cout << "entry " << ind << ": "
                 << list[ind].timestamp << ": "
                 << list[ind].isOriginal << ": "
                 << list[ind].filename << endl;
        }
    }

    isError = false;
}

LogFile :: ~LogFile(void)
{
    closeFile();

    if (zipHandles.size() > 0)
        cout << "\r\nwaiting for all zips to finish\r\n";

    while (zipHandles.size() > 0)
    {
        for (zipList::iterator it = zipHandles.begin();
             it != zipHandles.end(); it++)
        {
            ZipProcessHandle * zph = it->second;
            if (zph->getDone())
            {
                zipHandles.erase(it);
                delete zph;
            }
        }
        usleep(100000);
    }
}

void
LogFile :: nextLogFileName(void)
{
    counter++;
    ostringstream ostr;
    ostr << opts.logfileBase;
    if (opts.maxSizeSpecified)
        ostr << "." << setw(4) << setfill('0') << counter;
    currentLogFile = ostr.str();
}

static std::string
currentDate(void)
{
    char buf[64];
    struct tm current_time;
    time_t now;
    time(&now);
    localtime_r(&now, &current_time);
    memset(buf,0,sizeof(buf));
    strftime(buf,sizeof(buf)-1,"%Y/%m/%d %H:%M:%S", &current_time);
    return std::string(buf);
}

bool
LogFile :: openFile(void)
{
    if (currentStream != NULL)
        closeFile();
    nextLogFileName();
    currentStream = new ofstream(currentLogFile.c_str());
    if (!currentStream->good())
    {
        printError("open file");
        delete currentStream;
        currentStream = NULL;
    }
    else
    {
        currentSize = 0;
        (*currentStream) << "pfkscript log file opened "
                         << currentDate() << "\n\n";
    }
}

void
LogFile :: closeFile(void)
{
    if (currentStream != NULL)
    {
        (*currentStream) << "\n\npfkscript log file closed "
                         << currentDate() << endl;
        delete currentStream;
    }
    currentStream = NULL;
    trimFiles();
}

LogFile::logFileEnt::logFileEnt(const std::string &_fname, time_t _t,
                                bool _isOrig)
    : filename(_fname), timestamp(_t), isOriginal(_isOrig)
{
}

bool
LogFile::logFileEnt::sortTimestamp(const logFileEnt &a,
                                   const logFileEnt &b)
{
    // this sorts the list in reverse order, so the oldest
    // stuff is at the end of the list.
    return a.timestamp > b.timestamp;
}

void
LogFile :: globLogFiles(LogFile::LfeList &list)
{
    pfk_readdir   d(logDir.c_str());
    size_t baselen = logFilebase.length();
    if (d.ok())
    {
        struct dirent de;
        while (d.read(de))
        {
            string dirFileName(de.d_name);
            if (logFilebase.compare(0, baselen,
                                    dirFileName,
                                    0, baselen) == 0)
            {
                dirFileName = logDir + "/" + dirFileName;
                struct stat sb;
                if (stat(dirFileName.c_str(), &sb) == 0)
                {
                    bool isOrig = false;
                    size_t len = dirFileName.length();
                    if (len >= 6)
                    {
                        if (dirFileName[len-5] == '.' &&
                            isdigit(dirFileName[len-1]) &&
                            isdigit(dirFileName[len-2]) &&
                            isdigit(dirFileName[len-3]) &&
                            isdigit(dirFileName[len-4]))
                        {
                            isOrig = true;
                        }
                    }
                    list.push_back(logFileEnt(dirFileName,
                                              sb.st_mtime,
                                              isOrig));
                }
                else
                    printError("stat");
            }
                                         
        }
        std::sort(list.begin(), list.end(), logFileEnt::sortTimestamp);
    }
    else
        printError("opendir");
}

void
LogFile :: trimFiles(void)
{
    LfeList list;
    if (opts.maxFilesSpecified == false && opts.zipSpecified == false)
        return;

    globLogFiles(list);

    if (opts.maxFilesSpecified)
    {
        while (list.size() > opts.maxFiles)
        {
            // list is sorted in reverse order by timestamp so
            // start peeling off the end and removing files.
            const logFileEnt &lfe = list.back();
            unlink(lfe.filename.c_str());
            list.pop_back();
        }
    }

    if (opts.zipSpecified)
    {
        for (int ind = 0; ind < list.size(); ind++)
        {
            if (list[ind].isOriginal)
            {
                ZipProcessHandle * zph = new ZipProcessHandle(
                    opts, list[ind].filename);
                zph->createChild();
                zipHandles[zph->getPid()] = zph;
                // we let 'periodic' clean these up when 
                // they are done.
            }
        }
    }
}

void
LogFile :: periodic(void)
{
    if (currentStream)
        currentStream->flush();
    for (zipList::iterator it = zipHandles.begin();
         it != zipHandles.end(); it++)
    {
        ZipProcessHandle * zph = it->second;
        if (zph->getDone())
        {
            zipHandles.erase(it);
            delete zph;
        }
    }
}

void
LogFile :: addData(const char * data, size_t len)
{
    if (currentStream == NULL)
        openFile();
    if (currentStream != NULL)
    {
        currentStream->write(data,len);
        currentSize += len;
        if (opts.maxSizeSpecified && currentSize > opts.maxSize)
        {
            // max size reached, time to roll over
            closeFile();
            openFile();
        }
    }
}

/************************* ZipProcessHandle ******************************/

ZipProcessHandle :: ZipProcessHandle(const Options &_opts,
                                     const std::string &_fname)
    : opts(_opts), fname(_fname)
{
    tempInputFileName = fname + "_pfkscript_zipping_";
    tempOutputFileName = tempInputFileName;
    finalOutputFileName = fname;
    switch (opts.zipProgram)
    {
    case Options::ZIP_BZIP2:
        cmd.push_back( "bzip2" );
        tempOutputFileName += ".bz2";
        finalOutputFileName += ".bz2";
        break;
    case Options::ZIP_XZ:
        cmd.push_back( "xz" );
        tempOutputFileName += ".xz";
        finalOutputFileName += ".xz";
        break;
    case Options::ZIP_GZIP:
        // fallthru; if zipProgram weird, default to gzip.
    default:
        cmd.push_back( "gzip" );
        tempOutputFileName += ".gz";
        finalOutputFileName += ".gz";
        break;
    }
    rename(fname.c_str(), tempInputFileName.c_str());
    cmd.push_back( tempInputFileName.c_str());
    cmd.push_back( NULL );
    done = false;
}

ZipProcessHandle :: ~ZipProcessHandle(void)
{
    // nothing?
}

//virtual
void
ZipProcessHandle :: handleOutput(const char *buffer, size_t len)
{
    // compressors dont usually make much output
    ::write(1, buffer, len);
}

//virtual
void
ZipProcessHandle :: processExited(int status)
{
    if (0) // debug
    {
        cout << "compression completed for file " << fname
             << " (temp name " << tempInputFileName << ")" << endl;
        cout << "renaming " << tempOutputFileName
             << " to " << finalOutputFileName << endl;
    }
    done = true;
    rename(tempOutputFileName.c_str(), finalOutputFileName.c_str());
}
