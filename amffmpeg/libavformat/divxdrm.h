/********************************************************
 *
 *
 *
 *
 *
 *
 *
 * ********************************************************/

#ifndef DIVXDRM_H
#define DIVXDRM_H


#define OWNER_GUARD_BYTES               3
#define MODEL_ID_BYTES                  2
#define KEY_SIZE_BYTES                  16
#define SLOT_SERIAL_NUMBER_BYTES        2
#define OWNER_USER_ID_BYTES             5
#define DRM_ADP_RESERVED                8
#define TRANSACTION_ID_BYTES            46
#define DRM_OTHER_RESERVED              12
#define DRM_PORTABLE_KEY_LENGTH         44
#define DRM_FRAME_KEY_COUNT             128

typedef struct DrmActivateRecordStruct
{
      unsigned char memoryGuard[OWNER_GUARD_BYTES];
      unsigned char modelId[MODEL_ID_BYTES];
      unsigned char userKey[KEY_SIZE_BYTES];
      unsigned char reserved[3];
} DrmActivateRecord;

typedef struct DrmRentalRecordStruct
{
      unsigned short useLimitId;
      unsigned char serialNumber[SLOT_SERIAL_NUMBER_BYTES];
      unsigned char slotNumber;
      unsigned char reserved[3];
} DrmRentalRecord;

typedef struct DrmAdpTargetHeaderStruct
{
      unsigned short drmMode;
      unsigned char userId[OWNER_USER_ID_BYTES];
      unsigned char reservedAlign;
      DrmRentalRecord rentalRecord;
      unsigned char sessionKey[KEY_SIZE_BYTES];
      DrmActivateRecord activateRecord;
      unsigned char reserved[DRM_ADP_RESERVED];
} DrmAdpTargetHeader;

typedef struct DrmTransactionInfoHeaderStruct
{
      unsigned char transactionId[TRANSACTION_ID_BYTES];
      unsigned short transactionAuthorityId;
      unsigned contentId;
      unsigned char reserved[DRM_OTHER_RESERVED];
} DrmTransactionInfoHeader;

typedef struct DrmHeaderStruct
{
      unsigned keySourceMode;
      unsigned char masterKeyId[DRM_PORTABLE_KEY_LENGTH];
      DrmAdpTargetHeader adpTarget;
      DrmTransactionInfoHeader transaction;
      unsigned char frameKeys[DRM_FRAME_KEY_COUNT][KEY_SIZE_BYTES];
} DrmHeader;

typedef struct {
      unsigned   drm_offset;                              /* offset of encrypted video data */
      unsigned   drm_size;                                /* size of encrypted video data */
      unsigned   drm_check_value;                         /* check return value */
      unsigned   drm_rental_value;                      /* still can play count */
      DrmHeader *drm_header;                              /* const key informations */
      char      drm_reg_code[11];                 /* registrationCodeString*/
      
} drm_t;

typedef enum drmErrorCodes 
{
      DRM_SUCCESS = 0,
      DRM_NOT_AUTHORIZED,
      DRM_NOT_REGISTERED,
      DRM_RENTAL_EXPIRED,
      DRM_GENERAL_ERROR,
} drmErrorCodes_t;

void drm_set_info(drm_t*);
drm_t*  drm_get_info();

#endif

