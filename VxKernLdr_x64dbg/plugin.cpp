#include "plugin.h"
#include <windows.h>
#include <stdio.h>
#include <string>
#include "../VxKernLdr/VxKernLdr.h"

static DWORD pid = 0;
static bool hidden = false;
static std::string driverName = "VxKernLdr";

static ULONG GetVxKernLdrOptions()
{
    duint options = 0;
    if (!BridgeSettingGetUint("VxKernLdr", "Options", &options))
        options = 0xffffffff;
    return (ULONG)options;
}

static bool VxKernLdrCall(HIDE_COMMAND Command)
{
    auto path = "\\\\.\\" + driverName;
    HANDLE hDevice = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        _plugin_logputs("[" PLUGIN_NAME "] Could not open VxKernLdr handle (wrong driver name?)");
        return false;
    }
    HIDE_INFO HideInfo;
    HideInfo.Command = Command;
    HideInfo.Pid = pid;
    HideInfo.Type = GetVxKernLdrOptions();
    DWORD written = 0;
    auto result = false;
    if (WriteFile(hDevice, &HideInfo, sizeof(HIDE_INFO), &written, 0))
    {
        _plugin_logprintf("[" PLUGIN_NAME "] Process %shidden!\n", Command == UnhidePid ? "un" : "");
        result = true;
    }
    else
    {
        _plugin_logputs("[" PLUGIN_NAME "] WriteFile error...");
    }
    CloseHandle(hDevice);
    return result;
}

static bool cbVxKernLdr(int argc, char* argv[])
{
    if (!hidden)
    {
        _plugin_logprintf("[" PLUGIN_NAME "] Hiding PID %X (%ud)\n", pid, pid);
        if (VxKernLdrCall(HidePid))
        {
            DbgCmdExecDirect("hide");
            hidden = true;
        }
    }
    return hidden;
}

static bool cbTitanUnhide(int argc, char* argv[])
{
    if (hidden)
    {
        _plugin_logprintf("[" PLUGIN_NAME "] Unhiding PID %X (%ud)\n", pid, pid);
        if (VxKernLdrCall(UnhidePid))
            hidden = false;
    }
    return !hidden;
}

static bool cbVxKernLdrOptions(int argc, char* argv[])
{
    if (argc < 2)
    {
        _plugin_logprintf("[" PLUGIN_NAME "] Options: 0x%08X\n", GetVxKernLdrOptions());
    }
    else
    {
        duint options = DbgValFromString(argv[1]);
        BridgeSettingSetUint("VxKernLdr", "Options", options & 0xffffffff);
        if (hidden)
            VxKernLdrCall(HidePid);
        _plugin_logprintf("[" PLUGIN_NAME "] New options: 0x%08X\n", GetVxKernLdrOptions());
    }
    return true;
}

static bool cbVxKernLdrName(int argc, char* argv[])
{
    if (argc < 2)
    {
        _plugin_logprintf("[" PLUGIN_NAME "] Current driver name: '%s'\n", driverName.c_str());
    }
    else
    {
        driverName = argv[1];
        BridgeSettingSet("VxKernLdr", "DriverName", driverName.c_str());
        _plugin_logprintf("[" PLUGIN_NAME "] New driver name: '%s'\n", driverName.c_str());
    }
    return true;
}

PLUG_EXPORT void CBCREATEPROCESS(CBTYPE cbType, PLUG_CB_CREATEPROCESS* info)
{
    pid = info->fdProcessInfo->dwProcessId;
}

PLUG_EXPORT void CBATTACH(CBTYPE cbType, PLUG_CB_ATTACH* info)
{
    pid = info->dwProcessId;
}

PLUG_EXPORT void CBSYSTEMBREAKPOINT(CBTYPE cbType, PLUG_CB_SYSTEMBREAKPOINT* info)
{
    char* argv = "VxKernLdr";
    cbVxKernLdr(1, &argv);
}

PLUG_EXPORT void CBSTOPDEBUG(CBTYPE cbType, PLUG_CB_STOPDEBUG* info)
{
    char* argv = "TitanUnhide";
    cbTitanUnhide(1, &argv);
}

void VxKernLdrInit(PLUG_INITSTRUCT* initStruct)
{
    char setting[MAX_SETTING_SIZE] = "";
    BridgeSettingGet("VxKernLdr", "DriverName", setting);
    if (setting[0] != '\0')
    {
        driverName = setting;
    }

    _plugin_registercommand(pluginHandle, "VxKernLdr", cbVxKernLdr, true);
    _plugin_registercommand(pluginHandle, "TitanUnhide", cbTitanUnhide, true);
    _plugin_registercommand(pluginHandle, "VxKernLdrOptions", cbVxKernLdrOptions, false);
    _plugin_registercommand(pluginHandle, "VxKernLdrName", cbVxKernLdrName, false);
}

void VxKernLdrStop()
{
    _plugin_unregistercommand(pluginHandle, "VxKernLdrOptions");
    _plugin_unregistercommand(pluginHandle, "TitanUnhide");
    _plugin_unregistercommand(pluginHandle, "VxKernLdr");
}
