
#include LSBLK_OUTPUT_PROTO_HDR
#ifndef DEPENDING
#include "lsblk_converter.h"
#endif
#include "childprocessmanager.h"
#include "run_lsblk.h"
#include <string>

#define PATH_TO_LSBLK "/bin/lsblk"
#define LSBLK_ARGS "-abJO"

class LsblkHandler : public ChildProcessManager::Handle
{
    bool done;
    std::string  output;
    int exitStatus;
public:
    LsblkHandler(void)
    {
        done = false;
        exitStatus = 0;
        cmd.push_back(PATH_TO_LSBLK);
        cmd.push_back(LSBLK_ARGS);
        cmd.push_back(NULL);
    }
    virtual ~LsblkHandler(void)
    {
        // nothing?
    }
    /*virtual*/ void handleOutput(const char *buffer, size_t len)
    {
        output.append(buffer,len);
    }
    /*virtual*/ void processExited(int status)
    {
        exitStatus = status;
        done = true;
        printf("lsblk process exited with status %d\n", status);
    }
    bool getDone(int &status) const
    {
        status = 0;
        if (done == true)
            status = exitStatus;
        return done;
    }
    const std::string &getOutput(void) const
    {
        return output;
    }        
};

bool
run_lsblk(pfk::diskmanager::lsblk::BlockDevices_m &devs)
{
    bool ret = false;
    LsblkHandler h;
    ChildProcessManager::Manager * mgr =
        ChildProcessManager::Manager::instance();

    devs.Clear();
    if (mgr->createChild(&h) == false)
        ret = false;
    else
    {
        int status = 0;
        while (h.getDone(status) == false)
            usleep(1000);
        if (status == 0)
        {
            const std::string &output = h.getOutput();
//printf("output of lsblk:\n%s\n", output.c_str());
            SimpleJson::Property * ptree = SimpleJson::parseJson(output);

            if (ptree &&
                ptree->type == SimpleJson::Property::OBJECT)
            {
                SimpleJson::ObjectProperty * otree =
                    ptree->cast<SimpleJson::ObjectProperty>();
                if (JsonProtoConvert_BlockDevices_m(
                        devs, otree) == true)
                {
                    ret = true;
                }
            }

            if (ptree)
                delete ptree;
        }
    }

    ChildProcessManager::Manager::cleanup();
    return ret;
}
