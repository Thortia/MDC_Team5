/*
  Copyright (C) 2014 Parrot SA

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  * Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in
  the documentation and/or other materials provided with the
  distribution.
  * Neither the name of Parrot nor the names
  of its contributors may be used to endorse or promote products
  derived from this software without specific prior written
  permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
  AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  SUCH DAMAGE.
*/
/**
 * @file BebopSample.c
 * @brief This file contains sources about basic arsdk example sending commands to a bebop drone to pilot it,
 * receive its battery level and display the video stream.
 * @date 15/01/2015
 */

/*****************************************
 *
 *             include file :
 *
 *****************************************/

#include <stdlib.h>
#include <curses.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include <libARSAL/ARSAL.h>
#include <libARController/ARController.h>
#include <libARDiscovery/ARDiscovery.h>

#include "BebopSample.h"
#include "ihm.h"

/*****************************************
 *
 *             define :
 *
 *****************************************/
#define TAG "BebopSample"

#define ERROR_STR_LENGTH 2048

#define BEBOP_IP_ADDRESS "192.168.42.1"
#define BEBOP_DISCOVERY_PORT 44444

#define DISPLAY_WITH_MPLAYER 1

#define FIFO_DIR_PATTERN "/tmp/arsdk_XXXXXX"
#define FIFO_NAME "arsdk_fifo"

#define IHM

#define DEVICE_PORT     21
#define MEDIA_FOLDER    "internal_000"
/*****************************************
 *
 *             private header:
 *
 ****************************************/


/*****************************************
 *
 *             implementation :
 *
 *****************************************/
//
ARSAL_Thread_t threadRetreiveAllMedias;   // the thread that will do the media retrieving
ARSAL_Thread_t threadGetThumbnails;       // the thread that will download the thumbnails
ARSAL_Thread_t threadMediasDownloader;    // the thread that will download medias
ARDATATRANSFER_Manager_t *manager;        // the data transfer manager
ARUTILS_Manager_t *ftpListManager;        // an ftp that will do the list
ARUTILS_Manager_t *ftpQueueManager;       // an ftp that will do the download
//

static char fifo_dir[] = FIFO_DIR_PATTERN;
static char fifo_name[128] = "";

int gIHMRun = 1;
char gErrorStr[ERROR_STR_LENGTH];
IHM_t *ihm = NULL;

FILE *videoOut = NULL;
int frameNb = 0;
ARSAL_Sem_t stateSem;
int isBebop2 = 0;

static void signal_handler(int signal)
{
    gIHMRun = 0;
}

void createDataTransferManager() //데이터전송매니저 생성
{
	eARDATATRANSFER_ERROR result = ARDATATRANSFER_OK;
	manager = ARDATATRANSFER_Manager_New(&result);

	if (result == ARDATATRANSFER_OK)
	{
		eARUTILS_ERROR ftpError = ARUTILS_OK;
		ftpListManager = ARUTILS_Manager_New(&ftpError);
		if (ftpError == ARUTILS_OK)
		{
			ftpQueueManager = ARUTILS_Manager_New(&ftpError);
		}

		if (ftpError == ARUTILS_OK)
		{
			ftpError = ARUTILS_Manager_InitFtp(ftpListManager, discoveryDevice, ARUTILS_DESTINATION_DRONE, ARUTILS_FTP_TYPE_GENERIC);
		}

		if (ftpError == ARUTILS_OK)
		{
			ftpError = ARUTILS_Manager_InitFtp(ftpQueueManager, discoveryDevice, ARUTILS_DESTINATION_DRONE, ARUTILS_FTP_TYPE_GENERIC);
		}

		if (ftpError != ARUTILS_OK)
		{
			result = ARDATATRANSFER_ERROR_FTP;
		}
	}
	// NO ELSE

	if (result == ARDATATRANSFER_OK)
	{
		const char *path = "the/path/to/store/downloaded/data"; // Change according to your needs, or put as an argument

		result = ARDATATRANSFER_MediasDownloader_New(manager, ftpListManager, ftpQueueManager, MEDIA_FOLDER, path);
	}
}

void startMediaListThread()
{
	// first retrieve Medias without their thumbnails
	ARSAL_Thread_Create(&threadRetreiveAllMedias, ARMediaStorage_retreiveAllMediasAsync, NULL);
}

static void* ARMediaStorage_retreiveAllMediasAsync(void* arg)
{
	getAllMediaAsync();
	return NULL;
}

void getAllMediaAsync()
{
	eARDATATRANSFER_ERROR result = ARDATATRANSFER_OK;
	int mediaListCount = 0;

	if (result == ARDATATRANSFER_OK)
	{
		mediaListCount = ARDATATRANSFER_MediasDownloader_GetAvailableMediasSync(manager, 0, &result);
		if (result == ARDATATRANSFER_OK)
		{
			for (int i = 0; i < mediaListCount && result == ARDATATRANSFER_OK; i++)
			{
				ARDATATRANSFER_Media_t * mediaObject = ARDATATRANSFER_MediasDownloader_GetAvailableMediaAtIndex(manager, i, &result);
				printf("Media %i: %s", i, mediaObject->name);
				// Do what you want with this mediaObject
			}
		}
	}
}

//이하는 썸네일 전송 관련 코드
void startMediaThumbnailDownloadThread()
{
	// first retrieve Medias without their thumbnails
	ARSAL_Thread_Create(&threadGetThumbnails, ARMediaStorage_retreiveMediaThumbnailsSync, NULL);
}

static void* ARMediaStorage_retreiveMediaThumbnailsSync(void* arg)
{
	downloadThumbnails();
	return NULL;
}

void downloadThumbnails()
{
	ARDATATRANSFER_MediasDownloader_GetAvailableMediasAsync(manager, availableMediaCallback, NULL);
}

void availableMediaCallback(void* arg, ARDATATRANSFER_Media_t *media, int index)
{
	if (NULL != arg)
	{
		// The thumbnail image data is available in the media->thumbnail pointer.
		// The thumbnail data size is media->thumbnailSize
		// Do what you want with the image
	}
}

//이하는 본 파일 다운로드 관련 코드
void downloadMedias(ARDATATRANSFER_Media_t *medias, int count)
{
	eARDATATRANSFER_ERROR result = ARDATATRANSFER_OK;
	for (int i = 0; i < count && result == ARDATATRANSFER_OK; i++)
	{
		ARDATATRANSFER_Media_t *media = &medias[i];
		result = ARDATATRANSFER_MediasDownloader_AddMediaToQueue(manager, media, medias_downloader_progress_callback, NULL, medias_downloader_completion_callback, NULL);
	}

	if (result == ARDATATRANSFER_OK)
	{
		if (threadMediasDownloader == NULL)
		{
			// if not already started, start download thread in background
			ARSAL_Thread_Create(&threadMediasDownloader, ARDATATRANSFER_MediasDownloader_QueueThreadRun, manager);
		}
	}
}
void medias_downloader_progress_callback(void* arg, ARDATATRANSFER_Media_t *media, float percent)
{
	// the media is downloading
}

void medias_downloader_completion_callback(void* arg, ARDATATRANSFER_Media_t *media, eARDATATRANSFER_ERROR error)
{
	// the media is downloaded
}

//다운로드 취소 코드, 필요 없으면 삭제
void cancelCurrentDownload() {
	if (threadMediasDownloader != NULL)
	{
		ARDATATRANSFER_MediasDownloader_CancelQueueThread(manager);

		ARSAL_Thread_Join(threadMediasDownloader, NULL);
		ARSAL_Thread_Destroy(&threadMediasDownloader);
		threadMediasDownloader = NULL;
	}
}
void clean()
{
    if (threadRetreiveAllMedias != NULL)
    {
        ARDATATRANSFER_MediasDownloader_CancelGetAvailableMedias(manager);

        ARSAL_Thread_Join(threadRetreiveAllMedias, NULL);
        ARSAL_Thread_Destroy(&threadRetreiveAllMedias);
        threadRetreiveAllMedias = NULL;
    }

    if (threadGetThumbnails != NULL)
    {
        ARDATATRANSFER_MediasDownloader_CancelGetAvailableMedias(manager);

        ARSAL_Thread_Join(threadGetThumbnails, NULL);
        ARSAL_Thread_Destroy(&threadGetThumbnails);
        threadGetThumbnails = NULL;
    }

    if (threadMediasDownloader != NULL)
    {
        ARDATATRANSFER_MediasDownloader_CancelQueueThread(manager);

        ARSAL_Thread_Join(threadMediasDownloader, NULL);
        ARSAL_Thread_Destroy(&threadMediasDownloader);
        threadMediasDownloader = NULL;
    }

    ARDATATRANSFER_MediasDownloader_Delete(manager);

    ARUTILS_Manager_CloseWifiFtp(ftpListManager);
    ARUTILS_Manager_CloseWifiFtp(ftpQueueManager);

    ARUTILS_Manager_Delete(&ftpListManager);
    ARUTILS_Manager_Delete(&ftpQueueManager);
    ARDATATRANSFER_Manager_Delete(&manager);
}

int main (int argc, char *argv[])
{
    // local declarations
    int failed = 0;
    ARDISCOVERY_Device_t *device = NULL;
    ARCONTROLLER_Device_t *deviceController = NULL;
    eARCONTROLLER_ERROR error = ARCONTROLLER_OK;
    eARCONTROLLER_DEVICE_STATE deviceState = ARCONTROLLER_DEVICE_STATE_MAX;
    pid_t child = 0;

    /* Set signal handlers */
    struct sigaction sig_action = {
        .sa_handler = signal_handler,
    };

    int ret = sigaction(SIGINT, &sig_action, NULL);
    if (ret < 0)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, "ERROR", "Unable to set SIGINT handler : %d(%s)",
                    errno, strerror(errno));
        return 1;
    }
    ret = sigaction(SIGPIPE, &sig_action, NULL);
    if (ret < 0)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, "ERROR", "Unable to set SIGPIPE handler : %d(%s)",
                    errno, strerror(errno));
        return 1;
    }


    if (mkdtemp(fifo_dir) == NULL)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, "ERROR", "Mkdtemp failed.");
        return 1;
    }
    snprintf(fifo_name, sizeof(fifo_name), "%s/%s", fifo_dir, FIFO_NAME);

    if(mkfifo(fifo_name, 0666) < 0)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, "ERROR", "Mkfifo failed: %d, %s", errno, strerror(errno));
        return 1;
    }

    ARSAL_Sem_Init (&(stateSem), 0, 0);

    ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "Select your Bebop : Bebop (1) ; Bebop2 (2)");
    char answer = '1';
    scanf(" %c", &answer);
    if (answer == '2')
    {
        isBebop2 = 1;
    }

    if(isBebop2)
    {
        ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "-- Bebop 2 Sample --");
    }
    else
    {
        ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "-- Bebop Sample --");
    }

    if (!failed)
    {
        if (DISPLAY_WITH_MPLAYER)
        {
            // fork the process to launch mplayer
            if ((child = fork()) == 0)
            {
                execlp("xterm", "xterm", "-e", "mplayer", "-demuxer",  "h264es", fifo_name, "-benchmark", "-really-quiet", NULL);
                ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "Missing mplayer, you will not see the video. Please install mplayer and xterm.");
                return -1;
            }
        }

        if (DISPLAY_WITH_MPLAYER)
        {
            videoOut = fopen(fifo_name, "w");
        }
    }

#ifdef IHM
    ihm = IHM_New (&onInputEvent);
    if (ihm != NULL)
    {
        gErrorStr[0] = '\0';
        ARSAL_Print_SetCallback (customPrintCallback); //use a custom callback to print, for not disturb ncurses IHM

        if(isBebop2)
        {
            IHM_PrintHeader (ihm, "-- Bebop 2 Sample --");
        }
        else
        {
            IHM_PrintHeader (ihm, "-- Bebop Sample --");
        }
    }
    else
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, TAG, "Creation of IHM failed.");
        failed = 1;
    }
#endif

    // create a discovery device
    if (!failed)
    {
        ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "- init discovey device ... ");
        eARDISCOVERY_ERROR errorDiscovery = ARDISCOVERY_OK;

        device = ARDISCOVERY_Device_New (&errorDiscovery);

        if (errorDiscovery == ARDISCOVERY_OK)
        {
            ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "    - ARDISCOVERY_Device_InitWifi ...");
            // create a Bebop drone discovery device (ARDISCOVERY_PRODUCT_ARDRONE)

            if(isBebop2)
            {
                errorDiscovery = ARDISCOVERY_Device_InitWifi (device, ARDISCOVERY_PRODUCT_BEBOP_2, "bebop2", BEBOP_IP_ADDRESS, BEBOP_DISCOVERY_PORT);
            }
            else
            {
                errorDiscovery = ARDISCOVERY_Device_InitWifi (device, ARDISCOVERY_PRODUCT_ARDRONE, "bebop", BEBOP_IP_ADDRESS, BEBOP_DISCOVERY_PORT);
            }

            if (errorDiscovery != ARDISCOVERY_OK)
            {
                failed = 1;
                ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "Discovery error :%s", ARDISCOVERY_Error_ToString(errorDiscovery));
            }
        }
        else
        {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "Discovery error :%s", ARDISCOVERY_Error_ToString(errorDiscovery));
            failed = 1;
        }
    }

    // create a device controller
    if (!failed)
    {
        deviceController = ARCONTROLLER_Device_New (device, &error);

        if (error != ARCONTROLLER_OK)
        {
            ARSAL_PRINT (ARSAL_PRINT_ERROR, TAG, "Creation of deviceController failed.");
            failed = 1;
        }
        else
        {
            IHM_setCustomData(ihm, deviceController);
        }
    }

    if (!failed)
    {
        ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "- delete discovey device ... ");
        ARDISCOVERY_Device_Delete (&device);
    }

    // add the state change callback to be informed when the device controller starts, stops...
    if (!failed)
    {
        error = ARCONTROLLER_Device_AddStateChangedCallback (deviceController, stateChanged, deviceController);

        if (error != ARCONTROLLER_OK)
        {
            ARSAL_PRINT (ARSAL_PRINT_ERROR, TAG, "add State callback failed.");
            failed = 1;
        }
    }

    // add the command received callback to be informed when a command has been received from the device
    if (!failed)
    {
        error = ARCONTROLLER_Device_AddCommandReceivedCallback (deviceController, commandReceived, deviceController);

        if (error != ARCONTROLLER_OK)
        {
            ARSAL_PRINT (ARSAL_PRINT_ERROR, TAG, "add callback failed.");
            failed = 1;
        }
    }

    // add the frame received callback to be informed when a streaming frame has been received from the device
    if (!failed)
    {
        ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "- set Video callback ... ");
        error = ARCONTROLLER_Device_SetVideoStreamCallbacks (deviceController, decoderConfigCallback, didReceiveFrameCallback, NULL , NULL);

        if (error != ARCONTROLLER_OK)
        {
            failed = 1;
            ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "- error: %s", ARCONTROLLER_Error_ToString(error));
        }
    }

    if (!failed)
    {
        IHM_PrintInfo(ihm, "Connecting ...");
        ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "Connecting ...");
        error = ARCONTROLLER_Device_Start (deviceController);

        if (error != ARCONTROLLER_OK)
        {
            failed = 1;
            ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "- error :%s", ARCONTROLLER_Error_ToString(error));
        }
    }

    if (!failed)
    {
        // wait state update update
        ARSAL_Sem_Wait (&(stateSem));

        deviceState = ARCONTROLLER_Device_GetState (deviceController, &error);

        if ((error != ARCONTROLLER_OK) || (deviceState != ARCONTROLLER_DEVICE_STATE_RUNNING))
        {
            failed = 1;
            ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "- deviceState :%d", deviceState);
            ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "- error :%s", ARCONTROLLER_Error_ToString(error));
        }
    }

    // send the command that tells to the Bebop to begin its streaming
    if (!failed)
    {
        ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "- send StreamingVideoEnable ... ");
        error = deviceController->aRDrone3->sendMediaStreamingVideoEnable (deviceController->aRDrone3, 1);
        if (error != ARCONTROLLER_OK)
        {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "- error :%s", ARCONTROLLER_Error_ToString(error));
            failed = 1;
        }
    }

    if (!failed)
    {
        IHM_PrintInfo(ihm, "Running ... ('t' to takeoff ; Spacebar to land ; 'e' for emergency ; Arrow keys and ('r','f','d','g') to move ; 'q' to quit)");

#ifdef IHM
        while (gIHMRun)
        {
            usleep(50);
        }
#else
        int i = 20;
        ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "- sleep 20 ... ");
        while (gIHMRun && i--)
            sleep(1);
#endif
    }

#ifdef IHM
    IHM_Delete (&ihm);
#endif

    // we are here because of a disconnection or user has quit IHM, so safely delete everything
    if (deviceController != NULL)
    {


        deviceState = ARCONTROLLER_Device_GetState (deviceController, &error);
        if ((error == ARCONTROLLER_OK) && (deviceState != ARCONTROLLER_DEVICE_STATE_STOPPED))
        {
            IHM_PrintInfo(ihm, "Disconnecting ...");
            ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "Disconnecting ...");

            error = ARCONTROLLER_Device_Stop (deviceController);

            if (error == ARCONTROLLER_OK)
            {
                // wait state update update
                ARSAL_Sem_Wait (&(stateSem));
            }
        }

        IHM_PrintInfo(ihm, "");
        ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "ARCONTROLLER_Device_Delete ...");
        ARCONTROLLER_Device_Delete (&deviceController);

        if (DISPLAY_WITH_MPLAYER)
        {
            fflush (videoOut);
            fclose (videoOut);

            if (child > 0)
            {
                kill(child, SIGKILL);
            }
        }
    }

    ARSAL_Sem_Destroy (&(stateSem));

    unlink(fifo_name);
    rmdir(fifo_dir);

    ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "-- END --");

    return EXIT_SUCCESS;
}

/*****************************************
 *
 *             private implementation:
 *
 ****************************************/

// called when the state of the device controller has changed
void stateChanged (eARCONTROLLER_DEVICE_STATE newState, eARCONTROLLER_ERROR error, void *customData)
{
    ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "    - stateChanged newState: %d .....", newState);

    switch (newState)
    {
    case ARCONTROLLER_DEVICE_STATE_STOPPED:
        ARSAL_Sem_Post (&(stateSem));
        //stop
        gIHMRun = 0;

        break;

    case ARCONTROLLER_DEVICE_STATE_RUNNING:
        ARSAL_Sem_Post (&(stateSem));
        break;

    default:
        break;
    }
}

static void cmdBatteryStateChangedRcv(ARCONTROLLER_Device_t *deviceController, ARCONTROLLER_DICTIONARY_ELEMENT_t *elementDictionary)
{
    ARCONTROLLER_DICTIONARY_ARG_t *arg = NULL;
    ARCONTROLLER_DICTIONARY_ELEMENT_t *singleElement = NULL;

    if (elementDictionary == NULL) {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "elements is NULL");
        return;
    }

    // get the command received in the device controller
    HASH_FIND_STR (elementDictionary, ARCONTROLLER_DICTIONARY_SINGLE_KEY, singleElement);

    if (singleElement == NULL) {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "singleElement is NULL");
        return;
    }

    // get the value
    HASH_FIND_STR (singleElement->arguments, ARCONTROLLER_DICTIONARY_KEY_COMMON_COMMONSTATE_BATTERYSTATECHANGED_PERCENT, arg);

    if (arg == NULL) {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "arg is NULL");
        return;
    }

    // update UI
    batteryStateChanged(arg->value.U8);
}

static void cmdSensorStateListChangedRcv(ARCONTROLLER_Device_t *deviceController, ARCONTROLLER_DICTIONARY_ELEMENT_t *elementDictionary)
{
    ARCONTROLLER_DICTIONARY_ARG_t *arg = NULL;
    ARCONTROLLER_DICTIONARY_ELEMENT_t *dictElement = NULL;
    ARCONTROLLER_DICTIONARY_ELEMENT_t *dictTmp = NULL;

    eARCOMMANDS_COMMON_COMMONSTATE_SENSORSSTATESLISTCHANGED_SENSORNAME sensorName = ARCOMMANDS_COMMON_COMMONSTATE_SENSORSSTATESLISTCHANGED_SENSORNAME_MAX;
    int sensorState = 0;

    if (elementDictionary == NULL) {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "elements is NULL");
        return;
    }

    // get the command received in the device controller
    HASH_ITER(hh, elementDictionary, dictElement, dictTmp) {
        // get the Name
        HASH_FIND_STR (dictElement->arguments, ARCONTROLLER_DICTIONARY_KEY_COMMON_COMMONSTATE_SENSORSSTATESLISTCHANGED_SENSORNAME, arg);
        if (arg != NULL) {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "arg sensorName is NULL");
            continue;
        }

        sensorName = arg->value.I32;

        // get the state
        HASH_FIND_STR (dictElement->arguments, ARCONTROLLER_DICTIONARY_KEY_COMMON_COMMONSTATE_SENSORSSTATESLISTCHANGED_SENSORSTATE, arg);
        if (arg == NULL) {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, TAG, "arg sensorState is NULL");
            continue;
        }

        sensorState = arg->value.U8;
        ARSAL_PRINT(ARSAL_PRINT_INFO, TAG, "sensorName %d ; sensorState: %d", sensorName, sensorState);
    }
}

// called when a command has been received from the drone
void commandReceived (eARCONTROLLER_DICTIONARY_KEY commandKey, ARCONTROLLER_DICTIONARY_ELEMENT_t *elementDictionary, void *customData)
{
    ARCONTROLLER_Device_t *deviceController = customData;

    if (deviceController == NULL)
        return;

    // if the command received is a battery state changed
    switch(commandKey) {
    case ARCONTROLLER_DICTIONARY_KEY_COMMON_COMMONSTATE_BATTERYSTATECHANGED:
        cmdBatteryStateChangedRcv(deviceController, elementDictionary);
        break;
    case ARCONTROLLER_DICTIONARY_KEY_COMMON_COMMONSTATE_SENSORSSTATESLISTCHANGED:
        cmdSensorStateListChangedRcv(deviceController, elementDictionary);
        break;
    default:
        break;
    }
}

void batteryStateChanged (uint8_t percent)
{
    // callback of changing of battery level

    if (ihm != NULL)
    {
        IHM_PrintBattery (ihm, percent);
    }
}

eARCONTROLLER_ERROR decoderConfigCallback (ARCONTROLLER_Stream_Codec_t codec, void *customData)
{
    if (videoOut != NULL)
    {
        if (codec.type == ARCONTROLLER_STREAM_CODEC_TYPE_H264)
        {
            if (DISPLAY_WITH_MPLAYER)
            {
                fwrite(codec.parameters.h264parameters.spsBuffer, codec.parameters.h264parameters.spsSize, 1, videoOut);
                fwrite(codec.parameters.h264parameters.ppsBuffer, codec.parameters.h264parameters.ppsSize, 1, videoOut);

                fflush (videoOut);
            }
        }

    }
    else
    {
        ARSAL_PRINT(ARSAL_PRINT_WARNING, TAG, "videoOut is NULL.");
    }

    return ARCONTROLLER_OK;
}


eARCONTROLLER_ERROR didReceiveFrameCallback (ARCONTROLLER_Frame_t *frame, void *customData)
{
    if (videoOut != NULL)
    {
        if (frame != NULL)
        {
            if (DISPLAY_WITH_MPLAYER)
            {
                fwrite(frame->data, frame->used, 1, videoOut);

                fflush (videoOut);
            }
        }
        else
        {
            ARSAL_PRINT(ARSAL_PRINT_WARNING, TAG, "frame is NULL.");
        }
    }
    else
    {
        ARSAL_PRINT(ARSAL_PRINT_WARNING, TAG, "videoOut is NULL.");
    }

    return ARCONTROLLER_OK;
}


// IHM callbacks:

void onInputEvent (eIHM_INPUT_EVENT event, void *customData)
{
    // Manage IHM input events
    ARCONTROLLER_Device_t *deviceController = (ARCONTROLLER_Device_t *)customData;
    eARCONTROLLER_ERROR error = ARCONTROLLER_OK;

    switch (event)
    {
    case IHM_INPUT_EVENT_EXIT:
        IHM_PrintInfo(ihm, "IHM_INPUT_EVENT_EXIT ...");
        gIHMRun = 0;
        break;
    case IHM_INPUT_EVENT_EMERGENCY:
        if(deviceController != NULL)
        {
            // send a Emergency command to the drone
            error = deviceController->aRDrone3->sendPilotingEmergency(deviceController->aRDrone3);
        }
        break;
    case IHM_INPUT_EVENT_LAND:
        if(deviceController != NULL)
        {
            // send a takeoff command to the drone
            error = deviceController->aRDrone3->sendPilotingLanding(deviceController->aRDrone3);
        }
        break;
    case IHM_INPUT_EVENT_TAKEOFF:
        if(deviceController != NULL)
        {
            // send a landing command to the drone
            error = deviceController->aRDrone3->sendPilotingTakeOff(deviceController->aRDrone3);
        }
        break;
    case IHM_INPUT_EVENT_UP:
        if(deviceController != NULL)
        {
            // set the flag and speed value of the piloting command
            error = deviceController->aRDrone3->setPilotingPCMDGaz(deviceController->aRDrone3, 50);
        }
        break;
    case IHM_INPUT_EVENT_DOWN:
        if(deviceController != NULL)
        {
            error = deviceController->aRDrone3->setPilotingPCMDGaz(deviceController->aRDrone3, -50);
        }
        break;
    case IHM_INPUT_EVENT_RIGHT:
        if(deviceController != NULL)
        {
            error = deviceController->aRDrone3->setPilotingPCMDYaw(deviceController->aRDrone3, 50);
        }
        break;
    case IHM_INPUT_EVENT_LEFT:
        if(deviceController != NULL)
        {
            error = deviceController->aRDrone3->setPilotingPCMDYaw(deviceController->aRDrone3, -50);
        }
        break;
    case IHM_INPUT_EVENT_FORWARD:
        if(deviceController != NULL)
        {
            error = deviceController->aRDrone3->setPilotingPCMDPitch(deviceController->aRDrone3, 50);
            error = deviceController->aRDrone3->setPilotingPCMDFlag(deviceController->aRDrone3, 1);
        }
        break;
    case IHM_INPUT_EVENT_BACK:
        if(deviceController != NULL)
        {
            error = deviceController->aRDrone3->setPilotingPCMDPitch(deviceController->aRDrone3, -50);
            error = deviceController->aRDrone3->setPilotingPCMDFlag(deviceController->aRDrone3, 1);
        }
        break;
    case IHM_INPUT_EVENT_ROLL_LEFT:
        if(deviceController != NULL)
        {
            error = deviceController->aRDrone3->setPilotingPCMDRoll(deviceController->aRDrone3, -50);
            error = deviceController->aRDrone3->setPilotingPCMDFlag(deviceController->aRDrone3, 1);
        }
        break;
    case IHM_INPUT_EVENT_ROLL_RIGHT:
        if(deviceController != NULL)
        {
            error = deviceController->aRDrone3->setPilotingPCMDRoll(deviceController->aRDrone3, 50);
            error = deviceController->aRDrone3->setPilotingPCMDFlag(deviceController->aRDrone3, 1);
        }
        break;
	case IHM_INPUT_EVENT_CAPTURE: //캡쳐 기능
		if (deviceController != NULL)
		{
			error = deviceController->aRDrone3->sendMediaRecordPicture(deviceController->aRDrone3, 0);
		}
		break;
	case IHM_INPUT_EVENT_STARTRECORD: //녹화 시작 기능
		if (deviceController != NULL)
		{
			error = ARCONTROLLER_Device_StartVideoStream(deviceController);
		}
		break;
	case IHM_INPUT_EVENT_STOPRECORD: //녹화 정지 기능
		if (deviceController != NULL)
		{
			error = ARCONTROLLER_Device_StopVideoStream(deviceController);
		}
		break;
	case IHM_INPUT_EVENT_AUTOPYLOT: //자율주행 기능
		if (deviceController != NULL)
		{
			error = deviceController->aRDrone3->setPilotingPCMDRoll(deviceController->aRDrone3, );	
		}
		break;
	case IHM_INPUT_EVENT_SENDFILES: //사진, 영상데이터 전송 기능, 경로는 "internal_000/Bebop_Drone/media/"
		if (deviceController != NULL)
		{
			createDataTransferManager();
			startMediaListThread();
			startMediaThumbnailDownloadThread();

			clean();
		}
		break;
    case IHM_INPUT_EVENT_NONE:
        if(deviceController != NULL)
        {
            error = deviceController->aRDrone3->setPilotingPCMD(deviceController->aRDrone3, 0, 0, 0, 0, 0, 0);
        }
        break;
    default:
        break;
    }

    // This should be improved, here it just displays that one error occured
    if (error != ARCONTROLLER_OK)
    {
        IHM_PrintInfo(ihm, "Error sending an event");
    }
}

int customPrintCallback (eARSAL_PRINT_LEVEL level, const char *tag, const char *format, va_list va)
{
    // Custom callback used when ncurses is runing for not disturb the IHM

    if ((level == ARSAL_PRINT_ERROR) && (strcmp(TAG, tag) == 0))
    {
        // Save the last Error
        vsnprintf(gErrorStr, (ERROR_STR_LENGTH - 1), format, va);
        gErrorStr[ERROR_STR_LENGTH - 1] = '\0';
    }

    return 1;
}
