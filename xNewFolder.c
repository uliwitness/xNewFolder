// -----------------------------------------------------------------------------
//	Headers
// -----------------------------------------------------------------------------

#include <HyperXCmd.h>
#include <Types.h>
#include <Memory.h>
#include <OSUtils.h>
#include <ToolUtils.h>
#include <Files.h>
#include <Aliases.h>
#include <Errors.h>
#include <string.h>
#include <limits.h>
#include "XCmdUtils.h"
#include "MissingFiles.h"


// -----------------------------------------------------------------------------
//	Prototypes
// -----------------------------------------------------------------------------

OSErr GetVRefNumAndDirIDFromPath(CharsHandle filePath, short *vRefNum, long *dirID);


// -----------------------------------------------------------------------------
//	GetVRefNumAndDirIDFromPath
// -----------------------------------------------------------------------------

OSErr GetVRefNumAndDirIDFromPath(CharsHandle filePath, short *vRefNum, long *dirID) {
	OSErr		err = noErr;
	Str255		errStr = {0};
	AliasHandle	alias = NULL;
	Boolean		wasChanged = false;
	FSSpec		target = {0};
	CInfoPBRec	catInfo = {0};
	Str255		fileNameBuffer = {0};
		
	HLock(filePath);
	err = NewAliasMinimalFromFullPath(strlen(*filePath), *filePath, NULL, NULL, &alias);
	HUnlock(filePath);
	
	if (err != noErr) {
		NumToString(err, errStr);
		SetReturnValue("\pCan't locate parent folder at path: ");
		AppendReturnValue(errStr);
		return err;
	}
	
	err = ResolveAlias(NULL, alias, &target, &wasChanged);
	
	DisposeHandle((Handle)alias);
	alias = NULL;

	if (err != noErr) {
		NumToString(err, errStr);
		SetReturnValue("\pError getting folder info from path: ");
		AppendReturnValue(errStr);
		return err;
	} else if (target.vRefNum == 0) { // MoreFiles says this is a bug.
		SetReturnValue("\pCouldn't find volume.");
		return nsvErr;
	}
	
	memcpy(fileNameBuffer, target.name, target.name[0] + 1);	
	catInfo.dirInfo.ioVRefNum = target.vRefNum;
	catInfo.dirInfo.ioDrDirID = target.parID;
	catInfo.dirInfo.ioFDirIndex = 0;
	catInfo.dirInfo.ioNamePtr = fileNameBuffer;
	
	err = PBGetCatInfoSync(&catInfo);
	if (err != noErr) {
		NumToString(err, errStr);
		SetReturnValue("\pError getting folder ID from path: ");
		AppendReturnValue(errStr);
		return err;
	}
	
	*vRefNum = catInfo.dirInfo.ioVRefNum;
	*dirID = catInfo.dirInfo.ioDrDirID;
	
	return noErr;
}



// -----------------------------------------------------------------------------
//	xcmdmain
// -----------------------------------------------------------------------------

void xcmdmain(void)
{
	Str255 		errStr = {0};
	OSErr 		err = noErr;
	long 		dirID = 0;
	short 		fRefNum = 0;
	short 		vRefNum = 0;
	short 		dirIndex = 0;
	CharsHandle folderPath = NULL;
	CharsHandle folderPathParam = NULL;
	Str255		folderName;
	long 		createdDirID;
	long 		folderPathParamLen = 0;
	
	if ((folderPathParam = GetIndXParameter(1)) == NULL) {
		AppendReturnValue("\pExpected parent folder path as first parameter.");
		return;
	}
	
	folderPathParamLen = strlen(*folderPathParam);
	if (folderPathParamLen < 1) {
		AppendReturnValue("\pFirst parameter can't be empty.");
		return;
	}
	
	folderPath = NewHandle(folderPathParamLen + 2);
	BlockMove(*folderPathParam, *folderPath, folderPathParamLen + 1);
	
	if ((*folderPath)[folderPathParamLen - 1] != ':') {
		(*folderPath)[folderPathParamLen] = ':';
		(*folderPath)[folderPathParamLen + 1] = '\0';
	}
	
	if (!GetIndXParameter255(2, folderName)) {
		AppendReturnValue("\pExpected name for folder to create as second parameter.");
		return;
	}
	
	if (strcmp("?", *folderPath) == 0) {
		AppendReturnValue("\pSyntax: xNewFolder <parent folder path>, <new folder name>");
		return;
	} else if (strcmp("!", *folderPath) == 0) {
		AppendReturnValue("\p1.1, (c) Copyright 2021 by Uli Kusterer, all rights reserved.");
		return;
	}
	
	if (GetVRefNumAndDirIDFromPath(folderPath, &vRefNum, &dirID) != noErr) {
		return;
	}

	err = DirCreate(vRefNum, dirID, folderName, &createdDirID);
	if (err != noErr) {
		NumToString(err, errStr);
		SetReturnValue("\pError creating folder: ");
		AppendReturnValue(errStr);
	}
}