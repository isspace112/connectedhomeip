/*
 *
 *    <COPYRIGHT>
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *          Defines the public interface for the Device Layer SoftwareUpdateManager object.
 */

#ifndef SOFTWARE_UPDATE_MANAGER_H
#define SOFTWARE_UPDATE_MANAGER_H

#if CHIP_DEVICE_CONFIG_ENABLE_SOFTWARE_UPDATE_MANAGER

#include <platform/internal/CHIPDeviceLayerInternal.h>
#include <core/CHIPTLV.h>

namespace chip {
namespace DeviceLayer {

using namespace chip::TLV;

class SoftwareUpdateManagerImpl;

class SoftwareUpdateManager
{
    using ImplClass = SoftwareUpdateManagerImpl;


public:

    // ===== Members that define the public interface of the SoftwareUpdateManager

    enum State
    {
        kState_Idle                 = 1,
        kState_ScheduledHoldoff     = 2,
        kState_PrepareQuery         = 3,
        kState_Query                = 4,
        kState_PrepareImageStorage  = 5,
        kState_Download             = 6,
        kState_Install              = 7,

        kState_ApplicationManaged   = 8,

        kState_MaxState
    };

    /**
     * API events generated by the \c SoftwareUpdateManager object.
     */
    enum EventType
    {
        /**
         *  Prepare ImageQuery message
         *
         *  Generated when a software update check has been triggered. Provides an opportunity
         *  for the application to supply product related information to the
         *  SofwareUpdate:ImageQuery message.
         */
        kEvent_PrepareQuery,

        /**
         *  Prepare meta-data for ImageQuery request
         *
         *  Provides an opportunity for the application to append additional meta-data
         *  to the SofwareUpdate:ImageQuery message if needed. Generated when implementation
         *  is ready to get meta-data from the application.
         */
        kEvent_PrepareQuery_Metadata,

        /**
         *  Error preparing an ImageQuery request
         *
         *  Generated when the implementation encounters an error while preparing to send out
         *  a software update query.
         */
        kEvent_QueryPrepareFailed,

        /**
         *  ImageQuery request has been sent
         *
         *  Informational event to signal that a SofwareUpdate:ImageQuery message has been sent.
         */
        kEvent_QuerySent,

        /**
         *  Software update is available
         *
         *  Generated when a SofwareUpdate:ImageQueryResponse is received in response to
         *  a query containing information of the available update.
         */
        kEvent_SoftwareUpdateAvailable,

        /**
         *  Fetch persisted state information for a partially downloaded image
         *
         *  Provides an opportunity for the application to disclose information
         *  of a partial image previously downloaded so that the download
         *  may be continued from the point where it last stopped. URI of the available
         *  software update is provided as an input parameter that the application can use
         *  to compare if the image being downloaded is the same as the partial image.
         *
         *  The application is expected to return the length of the partial image in the
         *  PartialImageLenInBytes output parameter.  The application can set the value
         *  of PartialImageLenInBytes to 0 to indicate that no partial image exists or
         *  that the URI of the partial image does not match.
         *
         *  The application may choose to ignore this event by passing it to the default
         *  event handler. If this is done, the system will always download the entirety
         *  of the available firmware image.
         */
        kEvent_FetchPartialImageInfo,

        /**
         *  Prepare for storage of a new image
         *
         *  Requests the application to perform any steps necessary to prepare local storage
         *  for the download of a new firmware image.  The application can use this, for example,
         *  to erase flash pages.
         *
         *  The PrepareImageStorage event is generated only in the case where a new firmware
         *  image is being downloaded.  When a previously interrupted download is resumed,
         *  PrepareImageStorage is not generated.
         *
         *  The application must signal completion of the prepare operation by calling the
         *  \c PrepareImageStorageComplete() method.  It may do this within the event callback
         *  itself, or at a later time.  If called from a task other than the chip task,
         *  the caller must hold the chip stack lock.
         *
         *  The application can choose to ignore the PrepareImageStorage event by passing it
         *  to the default event handler.  If this is done, the system automatically proceeds
         *  to the image download state.
         *
         *  To support resuming an interrupted download, the application should persist the
         *  image URI (supplied as an event parameter), and use this when handling subsequent
         *  FetchPartialImageInfo events.
         */
        kEvent_PrepareImageStorage,

        /**
         *  Image download has begun
         *
         *  Informational event to signal the start of an image download transaction.
         */
        kEvent_StartImageDownload,

        /**
         *  Store a block of image data
         *
         *  Generated whenever a data block is received from the file download server.
         *  Parameters included with this event provide the data and the length of the data.
         *
         *  To support resuming an interrupted download, the application should maintain a
         *  persistent count of the total number of image bytes stored, and use this value
         *  when handling subsequent FetchPartialImageInfo events.
         */
        kEvent_StoreImageBlock,

        /**
         *  Compute an image integrity check value
         *
         *  Requests the application to compute an integrity check value over the downloaded
         *  image. Generated once downloading is complete.
         */
        kEvent_ComputeImageIntegrity,

        /**
         *  Reset state of partially downloaded image
         *
         *  Requests the application to forget the persisted state associated with a downloaded
         *  image.  A ResetPartialImageInfo event is generated whenever a downloaded image fails
         *  its integrity check.  After a ResetPartialImageInfo event has been processed,
         *  subsequent FetchPartialImageInfo events should indicate that no partial image is
         *  available.
         *
         *  Note that, when handling the ResetPartialImageInfo event, the application is NOT
         *  required to clear image data itself, only the state information associated with the
         *  image (i.e. the URI and partial image length).
         *
         *  If the application does not support image download resumption, it may ignore this
         *  event by passing it to the default event handler.
         */
        kEvent_ResetPartialImageInfo,

        /**
         *  Image is ready to be installed
         *
         *  Informational event to signal that image is ready to be installed.
         *  Generated once an image passes the integrity check.
         */
        kEvent_ReadyToInstall,

        /**
         *  Begin image installation
         *
         *  Requests the application to being the process of installing a downloaded firmware
         *  image.
         */
        kEvent_StartInstallImage,

        /**
         *  Software update process finished
         *
         *  Generated when a software update check has finished with or without
         *  errors. Parameters included with this event provide the reason for failure
         *  if the attempt finished due to a failure.
         */
        kEvent_Finished,

        /**
         *  Check default event handling behavior.
         *
         *  Used to verify correct default event handling in the application.
         *
         *  Applications must NOT handle this event.
         */
        kEvent_DefaultCheck                         = 100,

    };

    /**
     *  When a software update is available, the application can chose one of
     *  the following actions as part of the SoftwareUpdateAvailable API event
     *  callback. The default action will be set to kAction_Now.
     */
    enum ActionType
    {
        /**
         *  Ignore the download completely. A kEvent_Finished API event callback will
         *  be generated with error CHIP_DEVICE_ERROR_SOFTWARE_UPDATE_CANCELLED if
         *  this option is selected and the retry logic will not be invoked.
         */
        kAction_Ignore,

        /**
         *  Start the download right away. A kEvent_FetchPartialImageInfo API event
         *  callback will be generated right after.
         */
        kAction_DownloadNow,

        /**
         *  Pause download on start. Scheduled software update checks (if enabled) will be suspended.
         *  State machine will remain in Download state. When ready, application can
         *  call the resume download API to proceed with download or call Abort to cancel.
         */
        kAction_DownloadLater,

        /**
         *  Allows application to manage the rest of the phases of software update such as
         *  download, image integrity validation and install. Software update manager
         *  state machine will move to the ApplicationManaged state. Scheduled software update checks (if enabled)
         *  will be suspended till application calls Abort or ImageInstallComplete API.
         */
        kAction_ApplicationManaged,
    };

    /**
     *  Incoming parameters sent with events generated directly from this component
     *
     */
    union InEventParam;

   /**
     *  Outgoing parameters sent with events generated directly from this component
     *
     */
    union OutEventParam;

    struct RetryParam
    {
        /**
         *  Specifies the retry attempt number.
         *  It is reset on a successful software update attempt.
         */
        uint32_t NumRetries;
    };

    typedef void (*EventCallback)(void *apAppState, EventType aEvent, const InEventParam& aInParam, OutEventParam& aOutParam);
    typedef void (*RetryPolicyCallback)(void *aAppState, RetryParam& aRetryParam, uint32_t& aOutIntervalMsec);

    CHIP_ERROR Abort(void);
    CHIP_ERROR CheckNow(void);
    CHIP_ERROR ImageInstallComplete(CHIP_ERROR aError);
    CHIP_ERROR PrepareImageStorageComplete(CHIP_ERROR aError);
    CHIP_ERROR SetEventCallback(void * const aAppState, const EventCallback aEventCallback);
    CHIP_ERROR SetQueryIntervalWindow(uint32_t aMinWaitTimeMs, uint32_t aMaxWaitTimeMs);

    bool IsInProgress(void);

    void SetRetryPolicyCallback(const RetryPolicyCallback aRetryPolicyCallback);

    State GetState(void);

    static void DefaultEventHandler(void *apAppState, EventType aEvent,
                                    const InEventParam& aInParam,
                                    OutEventParam& aOutParam);

private:
    // ===== Members for internal use by the following friends.

    // friend class SoftwareUpdateManagerImpl;
    template<class> friend class Internal::GenericPlatformManagerImpl;

    CHIP_ERROR Init(void);

protected:

    // Construction/destruction limited to subclasses.
    SoftwareUpdateManager() = default;
    ~SoftwareUpdateManager() = default;

    // No copy, move or assignment.
    SoftwareUpdateManager(const SoftwareUpdateManager &) = delete;
    SoftwareUpdateManager(const SoftwareUpdateManager &&) = delete;
    SoftwareUpdateManager & operator=(const SoftwareUpdateManager &) = delete;
};

/**
 * Returns a reference to the public interface of the SoftwareUpdateManager singleton object.
 *
 * chip application should use this to access features of the SoftwareUpdateManager object
 * that are common to all platforms.
 */
extern SoftwareUpdateManager & SoftwareUpdateMgr(void);

/**
 * Returns the platform-specific implementation of the SoftwareUpdateManager singleton object.
 *
 * chip applications can use this to gain access to features of the SoftwareUpdateManager
 * that are specific to the selected platform.
 */
extern SoftwareUpdateManagerImpl & SoftwareUpdateMgrImpl(void);

} // namespace DeviceLayer
} // namespace chip

/* Include a header file containing the implementation of the SoftwareUpdateManager
 * object for the selected platform.
 */
#ifdef EXTERNAL_SOFTWAREUPDATEMANAGERIMPL_HEADER
#include EXTERNAL_SOFTWAREUPDATEMANAGERIMPL_HEADER
#else
#define SOFTWAREUPDATEMANAGERIMPL_HEADER <platform/SoftwareUpdateManagerImpl.h>
#include SOFTWAREUPDATEMANAGERIMPL_HEADER
#endif

namespace chip {
namespace DeviceLayer {

using namespace chip::TLV;

union SoftwareUpdateManager::InEventParam
{
    void Clear(void) { memset(this, 0, sizeof(*this)); }

    SoftwareUpdateManager * Source;
    struct
    {
        TLVWriter * MetaDataWriter;
    } PrepareQuery_Metadata;

    struct
    {
        CHIP_ERROR Error;
    } QueryPrepareFailed;

    struct
    {
        uint8_t IntegrityType;
        const char *URI;
        const char *Version;
    } SoftwareUpdateAvailable;

    struct
    {
        const char *URI;
    } FetchPartialImageInfo;

    struct
    {
        const char *URI;
        uint8_t IntegrityType;
    } PrepareImageStorage;

    struct
    {
        uint8_t *DataBlock;
        uint32_t DataBlockLen;
    } StoreImageBlock;

    struct
    {
        uint8_t IntegrityType;
        uint8_t *IntegrityValueBuf;     // Pointer to the buffer for the app to copy Integrity Value into.
        uint8_t IntegrityValueBufLen;   // Length of the provided buffer.
    } ComputeImageIntegrity;

    struct
    {
        CHIP_ERROR Error;
    } Finished;
};

union SoftwareUpdateManager::OutEventParam
{
    void Clear(void) { memset(this, 0, sizeof(*this)); }

    bool DefaultHandlerCalled;
    struct
    {
        const char *PackageSpecification;
        const char *DesiredLocale;
        CHIP_ERROR Error;
    } PrepareQuery;

    struct
    {
        CHIP_ERROR Error;
    } PrepareQuery_Metadata;

    struct
    {
        ActionType Action;
    } SoftwareUpdateAvailable;

    struct
    {
        uint64_t PartialImageLen;
    } FetchPartialImageInfo;

    struct
    {
        CHIP_ERROR Error;
    } StoreImageBlock;

    struct
    {
        CHIP_ERROR Error;
    } ComputeImageIntegrity;
};

inline CHIP_ERROR SoftwareUpdateManager::Init(void)
{
    return static_cast<ImplClass*>(this)->_Init();
}

inline CHIP_ERROR SoftwareUpdateManager::CheckNow(void)
{
    return static_cast<ImplClass*>(this)->_CheckNow();
}

inline CHIP_ERROR SoftwareUpdateManager::ImageInstallComplete(CHIP_ERROR aError)
{
    return static_cast<ImplClass*>(this)->_ImageInstallComplete(aError);
}

inline CHIP_ERROR SoftwareUpdateManager::PrepareImageStorageComplete(CHIP_ERROR aError)
{
    return static_cast<ImplClass*>(this)->_PrepareImageStorageComplete(aError);
}

inline SoftwareUpdateManager::State SoftwareUpdateManager::GetState(void)
{
    return static_cast<ImplClass*>(this)->_GetState();
}

inline CHIP_ERROR SoftwareUpdateManager::Abort(void)
{
    return static_cast<ImplClass*>(this)->_Abort();
}

inline bool SoftwareUpdateManager::IsInProgress(void)
{
    return static_cast<ImplClass*>(this)->_IsInProgress();
}

inline CHIP_ERROR SoftwareUpdateManager::SetQueryIntervalWindow(uint32_t aMinRangeSecs, uint32_t aMaxRangeSecs)
{
    return static_cast<ImplClass*>(this)->_SetQueryIntervalWindow(aMinRangeSecs, aMaxRangeSecs);
}

inline void SoftwareUpdateManager::SetRetryPolicyCallback(const RetryPolicyCallback aRetryPolicyCallback)
{
    static_cast<ImplClass*>(this)->_SetRetryPolicyCallback(aRetryPolicyCallback);
}

inline CHIP_ERROR SoftwareUpdateManager::SetEventCallback(void * const aAppState, const EventCallback aEventCallback)
{
    return static_cast<ImplClass*>(this)->_SetEventCallback(aAppState, aEventCallback);
}

inline void SoftwareUpdateManager::DefaultEventHandler(void *apAppState, EventType aEvent,
                                                       const InEventParam& aInParam,
                                                       OutEventParam& aOutParam)
{
    ImplClass::_DefaultEventHandler(apAppState, aEvent, aInParam, aOutParam);
}

} // namespace DeviceLayer
} // namespace chip

#endif // CHIP_DEVICE_CONFIG_ENABLE_SOFTWARE_UPDATE_MANAGER
#endif // SOFTWARE_UPDATE_MANAGER_H
