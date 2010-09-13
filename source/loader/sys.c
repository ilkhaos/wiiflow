#include <stdio.h>
#include <ogcsys.h>
#include <stdlib.h>
#include <wiiuse/wpad.h>
#include <malloc.h>
#include <string.h>
#include "sys.h"
#include "gecko.h"
#include "loader/playlog.h"
#include "sha1.h"
#include "ios_base.h"
#include "mload.h"

#define TITLE_ID(x,y)       (((u64)(x) << 32) | (y))
#define TITLE_HIGH(x)       ((u32)((x) >> 32))
#define TITLE_LOW(x)        ((u32)(x))

/* Constants */
#define CERTS_LEN	0x280

#define EXIT_TO_MENU 0
#define EXIT_TO_HBC 1
#define EXIT_TO_PRIILOADER 2

/* Variables */
static const char certs_fs[] ATTRIBUTE_ALIGN(32) = "/sys/cert.sys";
static bool reset = false;
static bool shutdown = false;

static bool return_to_hbc = false;
static bool return_to_menu = false;
static bool return_to_priiloader = false;

bool Sys_Exiting(void)
{
	return reset || shutdown;
}

void Sys_Test(void)
{
	if (reset)
		Sys_Reboot();
	else if (shutdown)
		Sys_Shutdown();
}

void Sys_ExitTo(int option, bool noHBC)
{
	return_to_hbc = option == EXIT_TO_HBC && !noHBC;
	return_to_menu = option == EXIT_TO_MENU;
	return_to_priiloader = option == EXIT_TO_PRIILOADER;
	
	//magic word to force wii menu in priiloader.
	if (return_to_hbc)
	{
		DCFlushRange((void*)0x8132fffb,4);
		*(vu32*)0x80001804 = 0x53545542;
		*(vu32*)0x80001808 = 0x48415858;
	}
	else if (return_to_menu)
	{
		DCFlushRange((void*)0x8132fffb,4);
		*(vu32*)0x8132fffb = 0x50756e65;
	}
	else if (return_to_priiloader)
	{
		DCFlushRange((void*)0x8132fffb,4);
		*(vu32*)0x8132fffb = 0x4461636f;
	}
}

void Sys_Exit(int ret)
{
	int i;
	for (i=0; i<4; i++)
	{
		WPAD_Flush(i);
		WPAD_Disconnect(i);
	}
    WPAD_Shutdown();

	Playlog_Delete();

	if (return_to_menu || return_to_priiloader)
		Sys_LoadMenu();
	else
		exit(ret);
}

void __Sys_ResetCallback(void)
{
	reset = true;
	/* Reboot console */
//	Sys_Reboot();
}

void __Sys_PowerCallback(void)
{
	shutdown = true;
	/* Poweroff console */
//	Sys_Shutdown();
}


void Sys_Init(void)
{
	/* Initialize video subsytem */
//	VIDEO_Init();

	/* Set RESET/POWER button callback */
	SYS_SetResetCallback(__Sys_ResetCallback);
	SYS_SetPowerCallback(__Sys_PowerCallback);
}

void Sys_Reboot(void)
{
	int i;
	for (i=0; i<4; i++)
	{
		WPAD_Flush(i);
		WPAD_Disconnect(i);
	}
	WPAD_Shutdown();

	/* Restart console */
	STM_RebootSystem();
}

void Sys_Shutdown(void)
{
	int i;
	for (i=0; i<4; i++)
	{
		WPAD_Flush(i);
		WPAD_Disconnect(i);
	}
	WPAD_Shutdown();

	/* Poweroff console */
	if(CONF_GetShutdownMode() == CONF_SHUTDOWN_IDLE)
	{
		s32 ret;

		/* Set LED mode */
		ret = CONF_GetIdleLedMode();
		if(ret >= 0 && ret <= 2)
			STM_SetLedMode(ret);

		/* Shutdown to idle */
		STM_ShutdownToIdle();
	}
	else
	{
		/* Shutdown to standby */
		STM_ShutdownToStandby();
	}
}

void Sys_LoadMenu(void)
{
	/* Return to the Wii system menu */
	SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
}


s32 Sys_GetCerts(signed_blob **certs, u32 *len)
{
	static signed_blob certificates[CERTS_LEN] ATTRIBUTE_ALIGN(32);

	s32 fd, ret;

	/* Open certificates file */
	fd = IOS_Open(certs_fs, 1);
	if (fd < 0)
		return fd;

	/* Read certificates */
	ret = IOS_Read(fd, certificates, sizeof(certificates));

	/* Close file */
	IOS_Close(fd);

	/* Set values */
	if (ret > 0)
	{
		*certs = certificates;
		*len   = sizeof(certificates);
	}

	return ret;
}

bool Sys_SupportsExternalModule(bool part_select)
{
//	u32 version = IOS_GetVersion();
	u32 revision = IOS_GetRevision();
	
	bool retval =  (part_select && is_ios_type(IOS_TYPE_WANIN) && revision >= 17) || (is_ios_type(IOS_TYPE_WANIN) && revision >= 18) || (is_ios_type(IOS_TYPE_HERMES) && revision >= 4);
	gprintf("IOS Version: %d, Revision %d, returning %d\n", IOS_GetVersion(), revision, retval);
	return retval;
}

int get_ios_type()
{
	switch (IOS_GetVersion()) {
		case 249:
		case 250:
			return IOS_TYPE_WANIN;
		case 222:
		case 223:
			if (IOS_GetRevision() == 1)
				return IOS_TYPE_KWIIRK;
			else
				return IOS_TYPE_HERMES;
		case 224:
			return IOS_TYPE_HERMES;
		default:
			return IOS_TYPE_WANIN;	
	}
	return IOS_TYPE_UNK;
}

int is_ios_type(int type)
{
	return (get_ios_type() == type);
}

s32 GetTMD(u64 TicketID, signed_blob **Output, u32 *Length)
{
    signed_blob* TMD = NULL;

    u32 TMD_Length;
    s32 ret;

    /* Retrieve TMD length */
    ret = ES_GetStoredTMDSize(TicketID, &TMD_Length);
    if (ret < 0)
        return ret;

    /* Allocate memory */
    TMD = (signed_blob*)memalign(32, (TMD_Length+31)&(~31));
    if (!TMD)
        return IPC_ENOMEM;

    /* Retrieve TMD */
    ret = ES_GetStoredTMD(TicketID, TMD, TMD_Length);
    if (ret < 0)
    {
        free(TMD);
        return ret;
    }

    /* Set values */
    *Output = TMD;
    *Length = TMD_Length;

    return 0;
}

s32 checkIOS(u32 IOS)
{
	signed_blob *TMD = NULL;
    tmd *t = NULL;
    u32 TMD_size = 0;
    u64 title_id = 0;
    s32 ret = 0;

    // Get tmd to determine the version of the IOS
    title_id = (((u64)(1) << 32) | (IOS));
    ret = GetTMD(title_id, &TMD, &TMD_size);

    if (ret == 0) {
        t = (tmd*)SIGNATURE_PAYLOAD(TMD);
		if (t->title_version == 65280) {
			ret = -1;
		}
    } else {
		ret = -2;
	}
    free(TMD);
    return ret;
}

s32 brute_tmd(tmd *p_tmd)
{
	u16 fill;
	for(fill=0; fill<65535; fill++)
	{
		p_tmd->fill3 = fill;
		sha1 hash;
		SHA1((u8 *)p_tmd, TMD_SIZE(p_tmd), hash);;
		if (hash[0]==0)
		{
			return 0;
		}
	}
	return -1;
}

char* get_ios_info_from_tmd()
{
	signed_blob *TMD = NULL;
	tmd *t = NULL;
	u32 TMD_size = 0;
	int try_ver = 0;
	u32 i;

	int ret = GetTMD((((u64)(1) << 32) | (IOS_GetVersion())), &TMD, &TMD_size);

	char *info = NULL;
	//static char default_info[32];
	//sprintf(default_info, "IOS%u (Rev %u)", IOS_GetVersion(), IOS_GetRevision());
	//info = (char *)default_info;

	if (ret != 0) goto out;

	t = (tmd*)SIGNATURE_PAYLOAD(TMD);

	// safety check
	if (t->title_id != TITLE_ID(1, IOS_GetVersion())) goto out;

	// patch title id, so hash matches
	if (t->title_id != TITLE_ID(1, 249)) {
		t->title_id = TITLE_ID(1,249);
		if (t->title_version >= 65530) { // for 250
			try_ver = t->title_version = 20;
		}
		retry_ver:
		// fake sign it
		brute_tmd(t);
	}

	sha1 hash;
	SHA1((u8 *)TMD, TMD_size, hash);

	for (i = 0; i < info_number; i++)
	{
		if (memcmp((void *)hash, (u32 *)&hashes[i], sizeof(sha1)) == 0)
		{
			info = (char *)&infos[i];
			break;
		}
	}
	if (info == NULL) {
		// not found, retry lower rev.
		if (try_ver > 13) {
			try_ver--;
			t->title_version = try_ver;
			goto retry_ver;
		}
	}

out:
	if(TMD != NULL)
	{
		free(TMD);
		TMD = NULL;
	}
    return info;
}

static char mload_ver_str[40];
static int mload_ver = 0;

void mk_mload_version()
{
	mload_ver_str[0] = 0;
	mload_ver = 0;
	if (is_ios_type(IOS_TYPE_HERMES) || (is_ios_type(IOS_TYPE_WANIN) && IOS_GetRevision() >= 18) )
	{
		if (IOS_GetRevision() >= 4) {
			if (is_ios_type(IOS_TYPE_WANIN)) {
				char *info = get_ios_info_from_tmd();
				if (info) {
						sprintf(mload_ver_str, "Base: IOS%s ", info);
				} else {
						sprintf(mload_ver_str, "Base: IOS?? DI:%d ", wanin_mload_get_IOS_base());
				}
			} else {
				sprintf(mload_ver_str, "Base: IOS%d ", mload_get_IOS_base());
			}
		}
		if (IOS_GetRevision() > 4) {
			int v, s;
			v = mload_ver = mload_get_version();
			s = v & 0x0F;
			v = v >> 4;
			sprintf(mload_ver_str + strlen(mload_ver_str), "mload v%d.%d ", v, s);
		} else {
			sprintf(mload_ver_str + strlen(mload_ver_str), "mload v%d ", IOS_GetRevision());
		}
	}
}

bool shadow_mload()
{
        if (!is_ios_type(IOS_TYPE_HERMES)) return false;
        int v51 = (5 << 4) & 1;
        if (mload_ver >= v51) {
                // shadow /dev/mload supported in hermes cios v5.1
                //IOS_Open("/dev/usb123/OFF",0);// this disables ehc completely
                IOS_Open("/dev/mload/OFF",0);
                return true;
        }
        return false;
}