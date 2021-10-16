/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */
/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

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

#include "posix_fe.h"

using namespace std;

void
printError(int e /*errno*/, const std::string &func)
{
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
    : opts(_opts)
{
    if (opts.maxSizeSpecified)
        counter = 1;
    else
        counter = 0;
    currentStream = NULL;
    stayClosed = false;
    initialized = false;
}

void
LogFile :: init(void)
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

    // look for previous log files, and if found,
    // find the next counter value.
    LfeList list;
    globLogFiles(list);

    int largest_counter = 0;
    for (auto le : list)
        if (le.numbered)
            if (largest_counter < le.count)
                largest_counter = le.count;
    if (largest_counter != 0)
        counter = largest_counter + 1;

    if (0) // debug
    {
        cout << "logdir = " << logDir << endl;
        cout << "logFilebase = " << logFilebase << endl;
        cout << "glob matches:\n";
        for (auto le : list)
            cout << le << endl;
    }
    initialized = true;
}

LogFile :: ~LogFile(void)
{
    if (!initialized)
        return;

    closeFile();

    if (zipHandles.size() > 0)
        cout << "\r\nwaiting for all zips to finish\r\n";

    while (zipHandles.size() > 0)
    {
        for (zipList::iterator it = zipHandles.begin();
             it != zipHandles.end();)
        {
            ZipProcessHandle * zph = it->second;
            if (zph->getDone())
            {
                it = zipHandles.erase(it);
                delete zph;
            }
            else
                it++;
        }
        usleep(100000);
    }
}

void
LogFile :: nextLogFileName(void)
{
    ostringstream ostr;
    ostr << logDir << "/" << logFilebase;
    if (counter != 0)
    {
        ostr << "." << setw(4) << setfill('0') << counter;
        counter++;
    }
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
        printError(errno, string("open file: ") + currentLogFile);
        delete currentStream;
        currentStream = NULL;
        return false;
    }
    // else

    currentSize = 0;
    (*currentStream) << "pfkscript log file opened "
                     << currentDate() << "\n\n";
    return true;
}

bool
LogFile :: closeFile(void)
{
    if (currentStream != NULL)
    {
        (*currentStream) << "\n\npfkscript log file closed "
                         << currentDate() << endl;
        delete currentStream;
    }
    currentStream = NULL;
    return trimFiles();
}

logFileEnt::logFileEnt(const std::string &_fname, time_t _t,
                                bool _numbered, bool _zipped,
                                int _count /*= 0*/)
    : filename(_fname), timestamp(_t),
      numbered(_numbered), zipped(_zipped), count(_count)
{
}

bool
logFileEnt::sortTimestamp(const logFileEnt &a,
                                   const logFileEnt &b)
{
    // this sorts the list in reverse order, so the oldest
    // stuff is at the end of the list.
    return a.timestamp > b.timestamp;
}

std::ostream &
operator<<(std::ostream &str, const logFileEnt &lfe)
{
    str << "log file entry: "
        << lfe.filename
        << ":"
        << lfe.timestamp;
    if (lfe.numbered)
        str << ":numbered:" << lfe.count;
    if (lfe.zipped)
        str << ":zipped";
    return str;
}

void
LogFile :: globLogFiles(LogFile::LfeList &list)
{
    pxfe_readdir  d;
    size_t baselen = logFilebase.length();
    if (d.open(logDir.c_str()))
    {
        struct dirent de;
        while (d.read(de))
        {
            string fileName(de.d_name);
            size_t fnLen = fileName.length();
            if (logFilebase.compare(0, baselen, fileName,
                                    0, baselen) == 0)
            {
                string dirFileName = logDir + "/" + fileName;
                struct stat sb;
                if (stat(dirFileName.c_str(), &sb) == 0)
                {
                    bool numbered = false;
                    bool zipped = false;
                    int countvalue = 0;

                    // enough room for ".%04d" ?
                    if (fnLen >= (baselen + 5))
                    {
                        char numStr[5];
                        memcpy(numStr,
                               fileName.c_str() + baselen + 1, 4);
                        numStr[4] = 0;
                        countvalue = atoi(numStr);
                        if (countvalue != 0)
                            numbered = true;
                        else
                            countvalue = 0;
                    }

                    // enough room for another period?
                    if (fnLen >= (baselen + 6))
                        if (fileName[baselen + 5] == '.')
                            zipped = true;

                    list.push_back(logFileEnt(dirFileName,
                                              sb.st_mtime,
                                              numbered, zipped,
                                              countvalue));
                }
                else
                    printError(errno, string("stat: ") + dirFileName);
            }
                                         
        }
        std::sort(list.begin(), list.end(), logFileEnt::sortTimestamp);
    }
    else
        printError(errno, string("opendir: ") + logDir);
}

bool
LogFile :: trimFiles(void)
{
    LfeList list;
    if (opts.maxFilesSpecified == false && opts.zipSpecified == false)
        return false;

    globLogFiles(list);

    if (opts.maxFilesSpecified)
    {
        while (list.size() > (uint32_t) opts.maxFiles)
        {
            // list is sorted in reverse order by timestamp so
            // start peeling off the end and removing files.
            const logFileEnt &lfe = list.back();
            unlink(lfe.filename.c_str());
            list.pop_back();
        }
    }

    bool ret = false;
    if (opts.zipSpecified)
    {
        for (uint32_t ind = 0; ind < list.size(); ind++)
        {
            if (list[ind].numbered && !list[ind].zipped)
            {
                ZipProcessHandle * zph = new ZipProcessHandle(
                    opts, list[ind].filename);
                zph->createChild();
                zipHandles[zph->getPid()] = zph;
                // we let 'periodic' clean these up when 
                // they are done.
                ret = true;
            }
        }
    }
    return ret;
}

void
LogFile :: periodic(FilenameList_t &list)
{
    if (currentStream)
        currentStream->flush();
    for ( zipList::iterator it = zipHandles.begin();
          it != zipHandles.end();)
    {
        if (it->second->getDone())
        {
            list.push_back(it->second->outputFilename());
            delete it->second;
            it = zipHandles.erase(it);
        }
        else
            it++;
    }
}

void
LogFile :: addData(const char * data, size_t len)
{
    if (currentStream == NULL)
    {
        if (stayClosed)
            return; // do nothing
        openFile();
    }
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

const bool
LogFile :: isOpen(void) const
{
    if (currentStream != NULL)
        return true;
    return false;
}
const std::string &
LogFile :: getFilename(void) const
{
    return currentLogFile;
}

bool
LogFile :: rolloverNow(void)
{
    if (counter == 0)
        counter = 1;
    stayClosed = false;
    bool ret = closeFile();
    openFile();
    return ret;
}

bool
LogFile :: closeNow(void)
{
    stayClosed = true;
    if (currentStream != NULL)
        return closeFile();
    return false;
}

void
LogFile :: openNow(void)
{
    if (counter == 0)
        counter = 1;
    stayClosed = false;
    if (currentStream == NULL)
        openFile();
}

/************************* ZipProcessHandle ******************************/

ZipProcessHandle :: ZipProcessHandle(const Options &_opts,
                                     const std::string &_fname)
    : opts(_opts), fname(_fname)
{
    //cout << "ZipProcessHandle created with fname " << fname << "\r\n";
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
    //cout << "ZipProcessHandle deleted with fname " << fname << "\r\n";
}

//virtual
void
ZipProcessHandle :: handleOutput(const char *buffer, size_t len)
{
    // compressors dont usually make much output
    if (::write(1, buffer, len) < 0)
        cerr << "handleOutput: write failed" << endl;
}

//virtual
void
ZipProcessHandle :: processExited(int status)
{
    if (0) // debug
    {
        cout << "compression completed for file " << fname
             << " (temp name " << tempInputFileName << ")" << "\r\n";
        cout << "renaming " << tempOutputFileName
             << " to " << finalOutputFileName << "\r\n";
    }
    done = true;
    rename(tempOutputFileName.c_str(), finalOutputFileName.c_str());
}
