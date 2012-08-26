/****************************************************************************
 * Copyright (C) 2012 FIX94
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/
#ifndef _GC_HPP_
#define _GC_HPP_

#include <gccore.h>

// DIOS-MIOS
typedef struct DML_CFG
{
	u32 Magicbytes;			//0xD1050CF6
	u32 CfgVersion;			//0x00000001
	u32 VideoMode;
	u32 Config;
	char GamePath[255];
	char CheatPath[255];
} DML_CFG;

enum dmlconfig
{
	DML_CFG_CHEATS		= (1<<0),
	DML_CFG_DEBUGGER	= (1<<1),
	DML_CFG_DEBUGWAIT	= (1<<2),
	DML_CFG_NMM			= (1<<3),
	DML_CFG_NMM_DEBUG	= (1<<4),
	DML_CFG_GAME_PATH	= (1<<5),
	DML_CFG_CHEAT_PATH	= (1<<6),
	DML_CFG_ACTIVITY_LED= (1<<7),
	DML_CFG_PADHOOK		= (1<<8),
	DML_CFG_NODISC_CFG1	= (1<<9),
	DML_CFG_FORCE_WIDE	= (1<<9), //v2
	DML_CFG_BOOT_DISC	= (1<<10),
	DML_CFG_BOOT_DOL	= (1<<11), //v1
	DML_CFG_BOOT_DISC2	= (1<<11), //v2
	DML_CFG_NODISC_CFG2	= (1<<12),
};

enum dmlvideomode
{
	DML_VID_DML_AUTO	= (0<<16),
	DML_VID_FORCE		= (1<<16),
	DML_VID_NONE		= (2<<16),

	DML_VID_FORCE_PAL50	= (1<<0),
	DML_VID_FORCE_PAL60	= (1<<1),
	DML_VID_FORCE_NTSC	= (1<<2),
	DML_VID_FORCE_PROG	= (1<<3),
	DML_VID_PROG_PATCH	= (1<<4),
};

void DML_New_SetOptions(const char *GamePath, char *CheatPath, const char *NewCheatPath, const char *partition, bool cheats, bool debugger, u8 NMM, u8 nodisc, u8 DMLvideoMode, u8 videoSetting, bool widescreen, bool new_dm_cfg);
void DML_Old_SetOptions(const char *GamePath);
void DML_New_SetBootDiscOption(bool new_dm_cfg);
void DML_New_WriteOptions();


// Devolution
typedef struct global_config
{
	u32 signature;			//0x3EF9DB23
	u16 version;			//0x00000100
	u16 device_signature;
	u32 memcard_cluster;
	u32 disc1_cluster;
	u32 disc2_cluster;
} gconfig;

bool DEVO_Installed(const char *path);
void DEVO_GetLoader(const char *loader);
void DEVO_SetOptions(const char *isopath, const char *partition, const char *gameID, bool memcard_emum);
void DEVO_Boot();

// General
void GC_SetVideoMode(u8 videomode, u8 videoSetting);
void GC_SetLanguage(u8 lang);
int GC_GameIsInstalled(char *discid, const char* partition, const char* dmlgamedir);

#endif //_GC_HPP_