//! Includes
#include "resource.h"

// Using the wonderful miniz by Rich Geldreich to add support for zip archives to orx
#include "miniz.c"


//! Variables

// Custom zip archive wrapper
typedef struct ZipArchive
{
  orxS32  s32Size, s32Cursor;
  orxU8  *pu8Buffer;

} ZipArchive;


//! Code

// --- Custom zip archive code ---

// Locate function, returns NULL if it can't handle the storage or if the resource can't be found in this storage
const orxSTRING orxFASTCALL ZipLocate(const orxSTRING _zStorage, const orxSTRING _zResource, orxBOOL _bRequireExistence)
{
  mz_zip_archive  stZipArchive;
  orxSTRING       zResult = orxNULL;
  static orxCHAR  acBuffer[512] = {0};

  // Clears zip archive memory
  orxMemory_Zero(&stZipArchive, sizeof(mz_zip_archive));

  // Can open zip file?
  if(mz_zip_reader_init_file(&stZipArchive, _zStorage, 0))
  {
    int iIndex;

    // Can locate content inside?
    if((iIndex = mz_zip_reader_locate_file(&stZipArchive, _zResource, NULL, 0)) >= 0)
    {
      // Creates location string: storage name + index of file
      orxString_NPrint(acBuffer, 511, "%s%c%d", _zStorage, orxRESOURCE_KC_LOCATION_SEPARATOR, iIndex);

      // Updates result
      zResult = acBuffer;
    }

    // Closes zip file
    mz_zip_reader_end(&stZipArchive);
  }

  // Done!
  return zResult;
}

// Open function: returns an opaque handle for subsequent function calls (GetSize, Seek, Tell, Read and Close) upon success, orxHANDLE_UNDEFINED otherwise
orxHANDLE orxFASTCALL ZipOpen(const orxSTRING _zLocation, orxBOOL _bEraseMode)
{
  orxS32    s32Separator;
  orxHANDLE hResult = orxHANDLE_UNDEFINED;

  // Not in erase mode and is this location correctly formated?
  if((_bEraseMode == orxFALSE)
  && ((s32Separator = orxString_SearchCharIndex(_zLocation, orxRESOURCE_KC_LOCATION_SEPARATOR, 0)) >= 0))
  {
    mz_zip_archive stZipArchive;

    // Clears separator for now
    *((orxSTRING)_zLocation + s32Separator) = orxCHAR_NULL;

    // Clears archive memory
    orxMemory_Zero(&stZipArchive, sizeof(mz_zip_archive));

    // Can open zip file?
    if(mz_zip_reader_init_file(&stZipArchive, _zLocation, 0))
    {
      orxS32 s32Index;

      // Gets content's index from location string
      orxString_ToS32(_zLocation + s32Separator + 1, &s32Index, orxNULL);

      // Valid?
      if(s32Index >= 0)
      {
        ZipArchive *pstZipArchive;

        // Allocates memory for our archive wrapper
        pstZipArchive = (ZipArchive *)orxMemory_Allocate(sizeof(ZipArchive), orxMEMORY_TYPE_MAIN);

        // Success?
        if(pstZipArchive != orxNULL)
        {
          mz_zip_archive_file_stat stStat;

          // Gets content's stat
          mz_zip_reader_file_stat(&stZipArchive, (mz_uint)s32Index, &stStat);

          // Stores its size
          pstZipArchive->s32Size = (orxS32)stStat.m_uncomp_size;

          // Allocates a buffer for storing uncompressed content
          pstZipArchive->pu8Buffer = (orxU8 *)orxMemory_Allocate(pstZipArchive->s32Size, orxMEMORY_TYPE_MAIN);

          // Success?
          if(pstZipArchive->pu8Buffer != orxNULL)
          {
            // Extracts content to the buffer
            if(mz_zip_reader_extract_to_mem(&stZipArchive, (mz_uint)s32Index, pstZipArchive->pu8Buffer, (size_t)stStat.m_uncomp_size, 0) != MZ_FALSE)
            {
              // Inits read cursor
              pstZipArchive->s32Cursor = 0;

              // Updates result
              hResult = (orxHANDLE)pstZipArchive;
            }
            else
            {
              // Frees our buffer and archive wrapper: extraction failure
              orxMemory_Free(pstZipArchive->pu8Buffer);
              orxMemory_Free(pstZipArchive);
            }
          }
          else
          {
            // Frees our archive wrapper: buffer allocation failure
            orxMemory_Free(pstZipArchive);
          }
        }
      }

      // Closes zip file
      mz_zip_reader_end(&stZipArchive);
    }

    // Restores separator in original location
    *((orxSTRING)_zLocation + s32Separator) = orxRESOURCE_KC_LOCATION_SEPARATOR;
  }

  // Done!
  return hResult;
}

// Close function: releases all that has been allocated in Open
void orxFASTCALL ZipClose(orxHANDLE _hResource)
{
  ZipArchive *pstZipArchive;

  // Gets archive wrapper
  pstZipArchive = (ZipArchive *)_hResource;

  // Frees its internal buffer
  orxMemory_Free(pstZipArchive->pu8Buffer);

  // Frees it
  orxMemory_Free(pstZipArchive);
}

// GetSize function: simply returns the size of the extracted resource, in bytes
orxS32 orxFASTCALL ZipGetSize(orxHANDLE _hResource)
{
  orxS32 s32Result;

  // Updates result
  s32Result = ((ZipArchive *)_hResource)->s32Size;

  // Done!
  return s32Result;
}

// Seek function: position the read cursor inside the data and returns the offset from start upon success or -1 upon failure
orxS32 orxFASTCALL ZipSeek(orxHANDLE _hResource, orxS32 _s32Offset, orxSEEK_OFFSET_WHENCE _eWhence)
{
  ZipArchive *pstZipArchive;
  orxS32      s32Cursor;

  // Gets archive wrapper
  pstZipArchive = (ZipArchive *)_hResource;

  // Depending on seek mode
  switch(_eWhence)
  {
    case orxSEEK_OFFSET_WHENCE_START:
    {
      // Computes cursor
      s32Cursor = _s32Offset;
      break;
    }

    case orxSEEK_OFFSET_WHENCE_CURRENT:
    {
      // Computes cursor
      s32Cursor = pstZipArchive->s32Cursor + _s32Offset;
      break;
    }

    case orxSEEK_OFFSET_WHENCE_END:
    {
      // Computes cursor
      s32Cursor = pstZipArchive->s32Size - _s32Offset;
      break;
    }

    default:
    {
      // Failure
      s32Cursor = -1;
      break;
    }
  }

  // Is cursor valid?
  if((s32Cursor >= 0) && (s32Cursor <= pstZipArchive->s32Size))
  {
    // Updates archive's cursor
    pstZipArchive->s32Cursor = s32Cursor;
  }

  // Done!
  return s32Cursor;
}

// Tell function: returns current read cursor
orxS32 orxFASTCALL ZipTell(orxHANDLE _hResource)
{
  orxS32 s32Result;

  // Updates result
  s32Result = ((ZipArchive *)_hResource)->s32Cursor;

  // Done!
  return s32Result;
}

// Read function: copies the requested amount of data, in bytes, to the given buffer and returns the amount of bytes copied
orxS32 orxFASTCALL ZipRead(orxHANDLE _hResource, orxS32 _s32Size, void *_pu8Buffer)
{
  ZipArchive *pstZipArchive;
  orxS32      s32CopySize, s32Result = 0;

  // Gets archive wrapper
  pstZipArchive = (ZipArchive *)_hResource;

  // Gets actual copy size to prevent any out-of-bound access
  s32CopySize = orxMIN(_s32Size, pstZipArchive->s32Size - pstZipArchive->s32Cursor);

  // Copies content
  orxMemory_Copy(_pu8Buffer, pstZipArchive->pu8Buffer + pstZipArchive->s32Cursor, s32CopySize);

  // Updates cursor
  pstZipArchive->s32Cursor += s32CopySize;

  // Done!
  return s32CopySize;
}

// --- End of custom zip archive code ---


orxSTATUS orxFASTCALL Init()
{
  orxRESOURCE_TYPE_INFO stInfo;
  orxSTATUS             eResult = orxSTATUS_SUCCESS;

  // Inits our zip resource wrapper
  orxMemory_Zero(&stInfo, sizeof(orxRESOURCE_TYPE_INFO));
  stInfo.zTag       = "zip";
  stInfo.pfnLocate  = ZipLocate;
  stInfo.pfnOpen    = ZipOpen;
  stInfo.pfnClose   = ZipClose;
  stInfo.pfnGetSize = ZipGetSize;
  stInfo.pfnSeek    = ZipSeek;
  stInfo.pfnTell    = ZipTell;
  stInfo.pfnRead    = ZipRead;
  stInfo.pfnWrite   = orxNULL; // No write support

  // Registers it
  eResult = orxResource_RegisterType(&stInfo);

  // Success?
  if(eResult != orxSTATUS_FAILURE)
  {
    // Loads data config
    orxConfig_Load("data.ini");

    // Creates viewport
    orxViewport_CreateFromConfig("Viewport");

    // Creates scene
    orxObject_CreateFromConfig("Scene");
  }

  // Done!
  return eResult;
}

orxSTATUS orxFASTCALL Run()
{
  orxVECTOR vMousePos;
  orxSTATUS eResult = orxSTATUS_SUCCESS;

  // Gets mouse world position
  if(orxRender_GetWorldPosition(orxMouse_GetPosition(&vMousePos), orxNULL, &vMousePos) != orxNULL)
  {
    // New status for Action input?
    if(orxInput_HasNewStatus("Action") != orxFALSE)
    {
      orxOBJECT *pstPickedObject;

      // Updates picking position
      vMousePos.fZ += orxFLOAT_1;

      // Picks object
      pstPickedObject = orxObject_Pick(&vMousePos);

      // Found?
      if(pstPickedObject != orxNULL)
      {
        // Pushes its config section
        orxConfig_PushSection(orxObject_GetName(pstPickedObject));

        // Action?
        if(orxInput_IsActive("Action") != orxFALSE)
        {
          // Has click track?
          if(orxConfig_HasValue("OnClickTrack") != orxFALSE)
          {
            // Adds it
            orxObject_AddTimeLineTrack(pstPickedObject, orxConfig_GetString("OnClickTrack"));
          }
        }
        else
        {
          // Has release track?
          if(orxConfig_HasValue("OnReleaseTrack"))
          {
            // Adds it
            orxObject_AddTimeLineTrack(pstPickedObject, orxConfig_GetString("OnReleaseTrack"));
          }
        }

        // Pops config section
        orxConfig_PopSection();
      }
    }
  }

  // Screenshot?
  if(orxInput_IsActive("Screenshot") && orxInput_HasNewStatus("Screenshot"))
  {
    // Captures it
    orxScreenshot_Capture();
  }
  // Quitting?
  if(orxInput_IsActive("Quit"))
  {
    // Updates result
    eResult = orxSTATUS_FAILURE;
  }

  // Done!
  return eResult;
}

void orxFASTCALL Exit()
{
  // We could delete everything we created here but orx will do it for us anyway =)
}

int main(int argc, char **argv)
{
  // Executes orx
  orx_Execute(argc, argv, Init, Run, Exit);

  // Done!
  return EXIT_SUCCESS;
}


#ifdef __orxWINDOWS__

#include "windows.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  // Executes orx
  orx_WinExecute(Init, Run, Exit);

  // Done!
  return EXIT_SUCCESS;
}

#endif // __orxWINDOWS__
