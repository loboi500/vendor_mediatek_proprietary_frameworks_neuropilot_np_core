/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2020. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

#define LOG_TAG "NpAgentService"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <fcntl.h>
#include <unistd.h>

#include "ModelFactory.h"
#include "NpAgentTypes.h"
#include "RuntimeOptions.h"
#include "hal/1.2/IAgent_V1_2.h"

using mediatek::neuropilotagent::IAgent_V1_2;

#ifdef __cplusplus
extern "C" {
#endif

void NpAgentServerInit() {
    LOG(INFO) << "NpAgentServerInit()";
    android::sp<IAgent_V1_2> npAgentService = new IAgent_V1_2();

    if (npAgentService->registerAsService() != android::NO_ERROR) {
        LOG(ERROR) << "Failed to register NeuroPilot Agent service";
    }

    const auto dlaPath = mediatek::neuropilotagent::RuntimeOptions().DlaPath();
    if (!dlaPath.empty()) {
        LOG(INFO) << "Try to load debug dla from " << dlaPath;
        int fd = open(dlaPath.c_str(), O_RDONLY | O_BINARY);
        if (fd < 0) {
            LOG(ERROR) << "Fail to open debug dla";
            return;
        }
        std::string actual;
        std::vector<uint32_t> params = {
            OP_POOL_SIZE,     7,    OP_BOOST_VALUE,       100, OP_INPUT_FORMAT,       0,
            OP_OUTPUT_FORMAT, 0,    OP_INPUT_COMPRESSION, 0,   OP_OUTPUT_COMPRESSION, 0,
            OP_INPUT_HEIGHT,  1600, OP_INPUT_WIDTH,       720, OP_OUTPUT_HEIGHT,      2400,
            OP_OUTPUT_WIDTH,  1080, OP_INPUT_W_STRIDE,    720, OP_OUTPUT_W_STRIDE,    1088};
        if (android::base::ReadFdToString(fd, &actual)) {
            int32_t agentId =
                mediatek::neuropilotagent::ModelFactory::GetInstance().PrepareModelFromCache(
                    fd, actual.size(), params);
            LOG(INFO) << "Prepare agent id " << agentId << " from " << dlaPath;
        } else {
            LOG(ERROR) << "Fail to load debug dla";
        }
        close(fd);
    }
}

void NpAgentServerDeInit() {}

#ifdef __cplusplus
}  //  extern "C"
#endif
