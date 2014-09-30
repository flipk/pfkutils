
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <math.h>
#include <ncurses/curses.h>
#include <fnmatch.h>

using namespace std;

#if XTDEBUG
FILE * debugfd;
#endif

struct dirItem;
typedef  map<string,dirItem*>  dirMap;
typedef vector<dirItem *>      dirVec;

struct dirItem {
    string fullPath;
    string filePart;
    bool selected;
    int statErr; // 0 if stat was okay
    struct stat sb; // only valid if statErr == 0
    bool isDir; // only valid if statErr == 0
    bool hasSubdirs; // only valid if isDir == true
    bool expanded; // only valid if isDir == true
    dirVec  contents; // only vlaid if isDir == true
    dirItem(const string &_fullPath)
        : fullPath(_fullPath)
    {
        hasSubdirs = false;
        expanded = false;
        selected = false;
        size_t slashPos = fullPath.find_last_of('/');
        if (slashPos == string::npos)
            filePart = fullPath;
        else
            filePart = fullPath.substr(slashPos+1);
        update();
    }
    void update(void)
    {
        if (lstat(fullPath.c_str(), &sb) < 0)
        {
            statErr = errno;
            isDir = false;
        }
        else
        {
            statErr = 0;
            if (S_ISDIR(sb.st_mode))
                isDir = true;
            else
                isDir = false;
        }
    }
    static bool compareDirItemPtrs(const dirItem *a, const dirItem *b)
    {
        return (*a < *b);
    }
    void updateDir(dirMap &allKnown)
    {
        contents.clear();
        if (isDir == false)
            return;
        DIR * d = opendir(fullPath.c_str());
        if (d)
        {
            struct dirent * de;
            while ((de = readdir(d)) != NULL)
            {
                string fname(de->d_name);
                if (fname == "." || fname == "..")
                    continue;
                string nfp = fullPath + "/" + fname;
                dirMap::iterator it = allKnown.find(nfp);
                dirItem * i = NULL;
                if (it == allKnown.end())
                {
                    i = new dirItem(nfp);
                    allKnown[nfp] = i;
                }
                else
                {
                    i = it->second;
                    i->update();
                }
                contents.push_back(i);
                if (i->isDir)
                    hasSubdirs = true;
            }
            closedir(d);
        }
        stable_sort(contents.begin(), contents.end(), compareDirItemPtrs);
    }
    bool operator<(const dirItem &other) const
    {
        return fullPath < other.fullPath;
    }
    static string bitsToMode(mode_t v)
    {
        string ret = "---";
        if (v & 4)   ret[0] = 'r';
        if (v & 2)   ret[1] = 'w';
        if (v & 1)   ret[2] = 'x';
        return ret;
    }
    string sbToString(void) const
    {
        // todo: make fields fixed width?
        ostringstream  ostr;
        if (statErr != 0)
        {
            ostr << strerror(statErr);
        }
        else if (S_ISLNK(sb.st_mode))
        {
            char target[500];
            int l = readlink(fullPath.c_str(), target, sizeof(target));
            ostr << "--> " << string(target,l);
        }
        else
        {
            mode_t m = sb.st_mode;
            char type = '?';
            if      (S_ISREG(m))  type = '-';
            else if (S_ISDIR(m))  type = 'd';
            else if (S_ISCHR(m))  type = 'c';
            else if (S_ISBLK(m))  type = 'b';
            else if (S_ISFIFO(m)) type = 'f';
            else if (S_ISLNK(m))  type = 'l';
            else if (S_ISSOCK(m)) type = 's';
            ostr << type;
            ostr << bitsToMode(m >> 6);
            ostr << bitsToMode(m >> 3);
            ostr << bitsToMode(m >> 0);
            ostr << " " << (int32_t) sb.st_nlink << " ";
            struct passwd * uname = getpwuid(sb.st_uid);
            if (uname)
                ostr << uname->pw_name << " ";
            else
                ostr << (int32_t) sb.st_uid << " ";
            struct group * gname = getgrgid(sb.st_gid);
            if (gname)
                ostr << gname->gr_name << " ";
            else
                ostr << (int32_t) sb.st_gid << " ";
            ostr << (int64_t) sb.st_size << " ";
            char timebuf[80];
            struct tm tmtime;
            localtime_r(&sb.st_mtime, &tmtime);
            strftime(timebuf,sizeof(timebuf), "%b %d %Y %H:%M:%S", &tmtime);
            ostr << timebuf;
        }
        return ostr.str();
    }
};

class xtreeWindow {
private:
    bool dotouch;
    enum panelmode_t {
        MODE_DIRS,
        MODE_FILES
    } mode;
    int numFileWinLines; // # lines on screen files&dirs can take up
    int fileWinLine; // line # on the screen where files&dirs start
    int finalCursorPosY; // line # on screen where cursor ended up
    string    startingDir;
    string    currentRoot;
    dirItem  dot;
    dirItem  dotdot;
    int currentDir;
    int dirStartPos;
    dirVec dirs; // what's displayed in left panel
    int currentFile;
    int fileStartPos;
    dirVec files; // what's displayed in right panel
    dirMap  allKnown; // official owner of all dirItem memory
    dirMap  selected; // all items user has selected.
    string dirMatchPattern;
    string fileMatchPattern;
    static const int max_spaces = 200;
    char _spaces[max_spaces];
    xtreeWindow(void)
        : dot("."), dotdot("..")
    {
        dotouch = true;
        mode = MODE_DIRS;
        initscr();
        raw();
        noecho();
        leaveok(stdscr,FALSE);
        {
            char currdir[512];
            getcwd(currdir, sizeof(currdir));
            startingDir = currdir;
        }
        updateRootDir(startingDir);
        refresh();
    }
    ~xtreeWindow(void)
    {
        clear();
        refresh();
        endwin();
        for (dirMap::iterator it = allKnown.begin();
             it != allKnown.end();
             it++)
        {
            dirItem * di = it->second;
            delete di;
        }
    }
    void updateRootDir(const string &newRoot)
    {
        if (dirs.size() > 2)
        {
            for (dirVec::iterator it = dirs.begin() + 2;
                 it != dirs.end();
                 it++)
            {
                (*it)->expanded = false;
            }
        }
        currentRoot = newRoot;
        dirItem * currentRootItem = NULL;
        dirMap::iterator it = allKnown.find(newRoot);
        if (it == allKnown.end())
        {
            currentRootItem = new dirItem(currentRoot);
            allKnown[currentRoot] = currentRootItem;
        }
        else
        {
            currentRootItem = it->second;
        }
        currentRootItem->updateDir(allKnown);
        dirs.clear();
        dirs.push_back(&dotdot);
        dirs.push_back(&dot);
        for (int dirInd = 0;
             dirInd < (int)currentRootItem->contents.size();
             dirInd++)
        {
            dirItem * di = currentRootItem->contents[dirInd];
            if (di->isDir)
            {
                di->updateDir(allKnown);
                dirs.push_back(di);
            }
        }
        currentDir = 1;
        dirStartPos = 0;
        currentFile = 0;
        fileStartPos = 0;
        dot.fullPath = currentRoot;
        dotdot.fullPath = currentRoot + "/..";
        selectDir();
    }
    char * spaces(int num)
    {
        if (num >= (max_spaces-1))
            num = max_spaces-1;
        memset(_spaces,' ',num);
        _spaces[num] = 0;
        return _spaces;
    }
    bool doMatchPattern(const string &fname, bool doDir)
    {
        const string * whichPattern =
            doDir ? &dirMatchPattern : &fileMatchPattern;
        if (whichPattern->size() == 0)
            return true;
        string realPattern;
        realPattern = "*" + *whichPattern + "*";
        int m = fnmatch(realPattern.c_str(), fname.c_str(), 0);
        if (m == FNM_NOMATCH)
            return false;
        if (m == 0)
            return true;
        return true;
    }
    void selectDir(void)
    {
        files.clear();
        currentFile = 0;
        fileStartPos = 0;
        dirMap::iterator it;
        if (currentDir < 2) // special "." or ".." entries
            it = allKnown.find(currentRoot);
        else
            it = allKnown.find(dirs[currentDir]->fullPath);
        if (it != allKnown.end())
        {
            dirItem * di = it->second;
            for (int diInd = 0;
                 diInd < (int)di->contents.size();
                 diInd++)
            {
                dirItem *di2 = di->contents[diInd];
                if (!di2->isDir)
                    if (doMatchPattern(di2->filePart,false))
                        files.push_back(di2);
            }
        }
    }
    int calcLevel(const dirItem *di)
    {
        int pos, level = 0;
        for (pos = currentRoot.length();
             pos < (int)di->fullPath.length();
             pos++)
        {
            if (di->fullPath[pos] == '/')
                level++;
        }

        return level;
    }
    void updatePane(const dirVec &vec, int startPos, int current,
                    panelmode_t whichPanel, const dirItem * &statItem,
                    int &finalCursorPosY, int &finalCursorPosX)
    {
        int linepos = 0;
        int pos;
        int startX = (whichPanel == MODE_DIRS) ? 2 : COLS/2+2;
        int lastPosDisplayed = 0;
        for (pos = startPos; linepos < numFileWinLines; pos++)
        {
            mvprintw(fileWinLine+linepos,startX-1,"%s",spaces(COLS/2-2));
            if (pos < (int)vec.size())
            {
                const dirItem *di = vec[pos];
                if (whichPanel == MODE_DIRS && pos > 1)
                {
                    bool matched = doMatchPattern(di->filePart,true);
                    if (matched == false)
                        continue; // xxx what does this break
                }
                if (mode == whichPanel && pos == current)
                {
                    finalCursorPosY = fileWinLine+linepos;
                    finalCursorPosX = startX;
                    statItem = di;
                }
                if (whichPanel == MODE_DIRS)
                    mvprintw(fileWinLine+linepos,startX-1,"%s%c",
                             spaces(calcLevel(di)),
                             di->hasSubdirs ? (di->expanded?'-':'+') : ' ');
                else
                    move(fileWinLine+linepos,startX);
                if (di->selected)
                    attron(A_REVERSE);
                printw("%s", di->filePart.substr(0,COLS/2-3).c_str());
                if (di->selected)
                    attroff(A_REVERSE);
                lastPosDisplayed = pos;
            }
            linepos++;
        }
    }
    void update(void)
    {
        int maxwidth = COLS - 10;
        int pathlines = (currentRoot.length() + maxwidth - 1) / maxwidth;
        fileWinLine = pathlines+2;
        numFileWinLines = LINES - fileWinLine - 5;
        if (dotouch)
        {
            clear();
            touchwin(stdscr);
            box(stdscr,0,0);
            mvprintw(1,2,"root:");
            move(pathlines+1,1);
            hline(0,COLS-2);
            move(LINES-3,1);
            hline(0,COLS-2);
            move(LINES-5,1);
            hline(0,COLS-2);
            move(fileWinLine,COLS/2);
            vline(0,numFileWinLines);
            dotouch = false;
        }
        for (int pathline=0; pathline < pathlines; pathline++)
        {
            string portion = currentRoot.substr(pathline*maxwidth,maxwidth);
            mvprintw(pathline+1,8,"%s",spaces(COLS-9));
            mvprintw(pathline+1,8,"%s",portion.c_str());
        }
        mvprintw(LINES-2,2, "%s", spaces(COLS-3));
        if (mode == MODE_DIRS)
            mvprintw(LINES-2,2,
                     "esc=cancel right=expand left=collapse ^X=exit ^U=clrptrn");
        else
            mvprintw(LINES-2,2,
                     "esc=cancel               space=toggle ^X=exit ^U=clrptrn");
        if (selected.size() > 0)
        {
            int width = (int) log10f((float) selected.size()) + 1;
            mvprintw(LINES-2, COLS-1-width, "%d", selected.size());
            mvprintw(LINES-2, COLS-10-width, "selected ");
            move(LINES-2, COLS-11-width);
            vline(0,1);
        }
        finalCursorPosY = 1;
        int finalCursorPosX = 1;
        const dirItem * statItem = NULL;
        updatePane(dirs, dirStartPos, currentDir, MODE_DIRS,
                   statItem, finalCursorPosY, finalCursorPosX);
        updatePane(files, fileStartPos, currentFile, MODE_FILES,
                   statItem, finalCursorPosY, finalCursorPosX);
        mvprintw(LINES-4,2,"%s",spaces(COLS-3));
        if (statItem)
            mvprintw(LINES-4,2,"%s",statItem->sbToString().c_str());
        if (dirMatchPattern.size() > 0 ||
            fileMatchPattern.size() > 0)
        {
            int filewidth = (int) fileMatchPattern.size();
            int dirwidth = (int) dirMatchPattern.size();
            mvprintw(LINES-4,COLS-1-filewidth, "%s",
                     fileMatchPattern.c_str());
            move(LINES-4,COLS-2-filewidth);
            vline(0,1);
            mvprintw(LINES-4,COLS-2-filewidth-dirwidth, "%s",
                     dirMatchPattern.c_str());
            move(LINES-4,COLS-3-filewidth-dirwidth);
            vline(0,1);
        }
        refresh();
        if (finalCursorPosY != 1 || finalCursorPosX != 1)
        {
            move(finalCursorPosY,finalCursorPosX-1);
            printw(">");
            refresh();
        }
        move(finalCursorPosY,finalCursorPosX-1);
        refresh();
    }
    bool valid_file_char(int c)
    {
        if (isalnum(c))
            return true;
        switch (c)
        {
        case '_':  case '*':  case '.':  case '?': case '\\':
            return true;
        }
        return false;
    }
    bool handle_char(int c)
    {
        if (valid_file_char(c))
        {
            if (mode == MODE_DIRS)
            {
                dirMatchPattern += (char) c;
                // if current dir pos doesn't match pattern,
                // then we need to move currentDir up until it does.
                while (currentDir > 1 &&
                       doMatchPattern(dirs[currentDir]->filePart,
                                      true) == false)
                {
                    currentDir--;
                }
            }
            else
            {
                fileMatchPattern += (char) c;
                selectDir();
            }
            return false;
        }
        int *startpos = (mode == MODE_DIRS) ? &dirStartPos : &fileStartPos;
        int *current = (mode == MODE_DIRS) ? &currentDir : &currentFile;
        int maxdata = (mode == MODE_DIRS) ? dirs.size() : files.size();
        switch (c)
        {
        case 8: // backspace (^H)
        case 127: // backspace (del, rubout)
            if (mode == MODE_DIRS)
            {
                if (dirMatchPattern.size() > 0)
                {
                    dirMatchPattern.resize(dirMatchPattern.size()-1);
                }
            }
            else
            {
                if (fileMatchPattern.size() > 0)
                {
                    fileMatchPattern.resize(fileMatchPattern.size()-1);
                    selectDir();
                }
            }
            break;
        case 21: // control U
            fileMatchPattern = "";
            dirMatchPattern = "";
            selectDir();
            break;
        case 24: // control X
            return true;
        case 27: // esc
            selected.clear();
            return true;
        case 12: // control L
        case KEY_RESIZE:
            dotouch = true;
            break;
        case 9: // tab
            mode = (mode == MODE_DIRS) ? MODE_FILES : MODE_DIRS;
            break;
        case 10: // enter
        {
            if (mode == MODE_FILES)
                break;
            // else mode == MODE_DIRS
            if (currentDir == 1) // "dot"
                // nothing to do.
                break;
            string newRoot;
            if (currentDir == 0) // "dotdot"
            {
                int lastSlash = currentRoot.find_last_of('/');
                if (lastSlash == 0)
                    newRoot = "/";
                else
                    newRoot = currentRoot.substr(0,lastSlash);
            }
            else
            {
                const string &selPath = dirs[currentDir]->fullPath;
                string pathRelRoot;
                if (selPath.compare(0,currentRoot.length(),currentRoot) == 0)
                    pathRelRoot = selPath.substr(currentRoot.length()+1);
                else
                    pathRelRoot = selPath;
                newRoot = currentRoot + "/" + pathRelRoot;
            }
            dirMatchPattern = "";
            updateRootDir(newRoot);
            break;
        }
        case KEY_DOWN:
            if (*current < (maxdata-1))
            {
                (*current) ++;
                if (finalCursorPosY == (fileWinLine+numFileWinLines-1))
                    (*startpos) ++ ;
            }
            if (mode == MODE_DIRS)
            {
                // if current dir pattern doesn't match,
                // keep moving down until we find one that does.
                while (currentDir < (dirs.size()-1) &&
                       doMatchPattern(dirs[currentDir]->filePart,
                                      true) == false)
                {
                    if (dirs[currentDir]->filePart == ".")
                        break;
                    currentDir++;
                }
                // if we hit bottom, go back up 
                while (currentDir > 1 &&
                       doMatchPattern(dirs[currentDir]->filePart,
                                      true) == false)
                {
                    currentDir--;
                }
                selectDir();
            }
            break;
        case KEY_UP:
            if (*current > 0)
            {
                (*current) --;
                if (finalCursorPosY == fileWinLine)
                    (*startpos) --;
            }
            if (mode == MODE_DIRS)
            {
                // if current dir pattern doesn't match,
                // move up until we find one that does.
                while (currentDir > 1 &&
                       doMatchPattern(dirs[currentDir]->filePart,
                                      true) == false)
                {
                    currentDir--;
                }
                selectDir();
            }
            break;
        case KEY_NPAGE:
        {
            // xxx handle
            break;
        }
        case KEY_PPAGE:
        {
            // xxx handle
            break;
        }
        case KEY_RIGHT:
        {
            if (mode == MODE_FILES)
                break;
            dirItem * di = dirs[currentDir];
            if (di->hasSubdirs && !di->expanded)
            {
                for (int ind = 0;
                     ind < (int)di->contents.size();
                     ind++)
                {
                    dirItem * di2 = di->contents[ind];
                    if (!di2->isDir)
                        continue;
                    di2->updateDir(allKnown);
                    dirVec::iterator it = dirs.begin();
                    dirs.insert(it+(currentDir+1), di2);
                }
                di->expanded = true;
            }
            break;
        }
        case KEY_LEFT:
        {
            if (mode == MODE_FILES)
                break;
            dirItem * di = dirs[currentDir];
            if (di->expanded)
            {
                dirVec::iterator it = dirs.begin() + currentDir + 1;
                const string &subdirpath = di->fullPath;
                for (; it != dirs.end();)
                {
                    dirItem * di2 = *it;
                    if (di2->fullPath.compare(
                            0, subdirpath.length(), subdirpath) != 0)
                        break;
                    di2->expanded = false;
                    it = dirs.erase(it);
                }
                di->expanded = false;
            }
            break;
        }
        case ' ': // space
        {
            dirItem * di = NULL;
            if (mode == MODE_DIRS)
                di = currentDir < (int)dirs.size() ? dirs[currentDir] : NULL;
            else
                di = currentFile < (int)files.size() ? files[currentFile] : NULL;
            if (di)
            {
                if (di->selected)
                {
                    di->selected = false;
                    dirMap::iterator it = selected.find(di->fullPath);
                    if (it != selected.end())
                        selected.erase(it);
                }
                else
                {
                    di->selected = true;
                    selected[di->fullPath] = di;
                }
            }
            break;
        }
        }
        return false;
    }
    // ncurses in 'keypad' mode will give me ESC but only
    // after a long, long delay. so, do it myself.
    int mygetch(void)
    {
    again:
        int c = getch();
        if (c == 27)
        {
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(0,&rfds);
            struct timeval tv = { 0, 100000 };
            int selret = select(1,&rfds,NULL,NULL,&tv);
            if (selret > 0)
            {
                switch (getch())
                {
                case '[': // arrows and pgup/pgdown
                {
                    switch (getch())
                    {
                    case 'A':  c = KEY_UP;     break;
                    case 'B':  c = KEY_DOWN;   break;
                    case 'C':  c = KEY_RIGHT;  break;
                    case 'D':  c = KEY_LEFT;   break;
                    case '2': // insert, wait for 7e
                    {
                        c = KEY_IC;
                        break;
                    }
                    case '3': // delete, wait for 7e
                    {
                        c = KEY_DC;
                        break;
                    }
                    case '5': // pgup, wait for 7e
                    {
                        c = KEY_PPAGE;
                        break;
                    }
                    case '6': // pgdown, wait for 7e
                    {
                        c = KEY_NPAGE;
                        break;
                    }
                    default: goto again;
                    }
                    break;
                }
                case 'O': // home/end
                {
                    switch (getch())
                    {
                    case 'H': // home
                        c = KEY_HOME;
                        break;
                    case 'F': // end
                        c = KEY_END;
                        break;
                    default: goto again;
                    }
                    break;
                }
                }
            }
        }
        return c;
    }
    void eventLoop(void)
    {
        int c;
        bool done;
        do {
            update();
            c = mygetch();
            done = handle_char(c);
        } while (!done);
    }
    vector<string> getSelected(void)
    {
        vector<string> ret;
        dirMap::iterator it;
        for (it = selected.begin(); it != selected.end(); it++)
        {
            string ret1;
            if (it->second->fullPath.compare(0,
                                             startingDir.length(),
                                             startingDir) == 0)
            {
                ret.push_back(
                    it->second->fullPath.substr(startingDir.length()+1));
            }
            else
            {
                ret.push_back(
                    it->second->fullPath);
            }
        }
        return ret;
    }
public:
    static vector<string> run(void)
    {
        xtreeWindow win;
        win.eventLoop();
        return win.getSelected();
    }
};

extern "C"
char **
xtree_get_selection(int *num)
{
    vector<string> sel = xtreeWindow::run();
    int selsize = (int) sel.size();
    *num = selsize;
    char ** ret = (char**) malloc( sizeof(char*) * selsize );
    for (int ind = 0; ind < selsize; ind++)
    {
        int len = sel[ind].length() + 1;
        ret[ind] = (char*) malloc(len);
        memcpy(ret[ind], sel[ind].c_str(), len);
    }
    return ret;
}
