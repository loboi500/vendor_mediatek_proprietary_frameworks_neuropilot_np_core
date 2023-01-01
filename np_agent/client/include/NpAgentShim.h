/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 *
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

#ifndef __NEUROPILOTAGENT_SHIM_H__
#define __NEUROPILOTAGENT_SHIM_H__

#include <android/log.h>
#include <cutils/native_handle.h>
#include <dlfcn.h>
#include <system/graphics-base-v1.0.h>

/**
 * Buffer compression mode.
 */
typedef enum {
    COMPRESSION_NONE = 0,
    COMPRESSION_AFBC = 1,
} BufferCompressionMode;

/**
 * Result codes.
 */
typedef enum {
    RESULT_NO_ERROR = 0,
    RESULT_OUT_OF_MEMORY = 1,
    RESULT_INCOMPLETE = 2,
    RESULT_UNEXPECTED_NULL = 3,
    RESULT_BAD_DATA = 4,
    RESULT_OP_FAILED = 5,
    RESULT_UNMAPPABLE = 6,
    RESULT_BAD_STATE = 7,
    RESULT_BAD_VERSION = 8,
} ResultCode;

typedef enum {
    OPTION_POOL_SIZE = 1,
    OPTION_BOOST_VALUE = 1 << 2,
    OPTION_INPUT_FORMAT = 1 << 3,
    OPTION_OUTPUT_FORMAT = 1 << 4,
    OPTION_INPUT_COMPRESSION = 1 << 5,
    OPTION_OUTPUT_COMPRESSION = 1 << 6,
    OPTION_INPUT_HEIGHT = 1 << 7,
    OPTION_INPUT_WIDTH = 1 << 8,
    OPTION_OUTPUT_HEIGHT = 1 << 9,
    OPTION_OUTPUT_WIDTH = 1 << 10,
    OPTION_INPUT_W_STRIDE = 1 << 11,
    OPTION_OUTPUT_W_STRIDE = 1 << 12,
} OptionCode;

#define NPAGENT_LOG_D(format, ...) \
    __android_log_print(ANDROID_LOG_DEBUG, "NpAgentShim", format "\n", ##__VA_ARGS__);

#define NPAGENT_LOG_E(format, ...) \
    __android_log_print(ANDROID_LOG_ERROR, "NpAgentShim", format "\n", ##__VA_ARGS__);

#define LOAD_NPAGENT_FUNCTION(name) \
    static name##_fn fn = reinterpret_cast<name##_fn>(loadNpAgentFunction(#name));

#define EXECUTE_NPAGENT_FUNCTION(...) \
    if (fn != nullptr) {              \
        fn(__VA_ARGS__);              \
    }

#define EXECUTE_NPAGENT_FUNCTION_RETURN_INT(...) \
    return fn != nullptr ? fn(__VA_ARGS__) : RESULT_NO_ERROR;

#define EXECUTE_NPAGENT_FUNCTION_RETURN_BOOL(...) return fn != nullptr ? fn(__VA_ARGS__) : false;

#define EXECUTE_NPAGENT_FUNCTION_RETURN_POINTER(...) \
    return fn != nullptr ? fn(__VA_ARGS__) : nullptr;

/*
 * NpAgentOptions is an opaque type that contains the options to configure
 * the NeuroPilot agent.
 */
typedef struct NpAgentOptions NpAgentOptions;

/*
 * NpAgentAttributes is an opaque type that contains the information of
 * the NeuroPilot agent.
 */
typedef struct NpAgentAttributes NpAgentAttributes;

/*
 * NpAgentExecution is an opaque type that represents the execution
 */
typedef struct NpAgentExecution NpAgentExecution;

/*************************************************************************************************/
typedef int (*NpAgentOptions_create_fn)(NpAgentOptions** options);
typedef void (*NpAgentOptions_release_fn)(NpAgentOptions* options);
typedef int (*NpAgentOptions_setPoolSize_fn)(NpAgentOptions* options, uint32_t size);
typedef int (*NpAgentOptions_setBoostValue_fn)(NpAgentOptions* options, uint32_t value);
typedef int (*NpAgentOptions_setInputFormat_fn)(NpAgentOptions* options, uint32_t format);
typedef int (*NpAgentOptions_setOutputFormat_fn)(NpAgentOptions* options, uint32_t format);
typedef int (*NpAgentOptions_setInputCompressionMode_fn)(NpAgentOptions* options, uint32_t mode);
typedef int (*NpAgentOptions_setOutputCompressionMode_fn)(NpAgentOptions* options, uint32_t mode);
typedef int (*NpAgentOptions_setInputHeightWidth_fn)(NpAgentOptions* options, uint32_t height,
                                                     uint32_t width);
typedef int (*NpAgentOptions_setOutputHeightWidth_fn)(NpAgentOptions* options, uint32_t height,
                                                      uint32_t width);
typedef int (*NpAgentOptions_setInputStride_fn)(NpAgentOptions* options, uint32_t stride);
typedef int (*NpAgentOptions_setOutputStride_fn)(NpAgentOptions* options, uint32_t stride);
typedef int (*NpAgent_createFromBuffer_fn)(const void* buffer, size_t size,
                                           NpAgentOptions* options);
typedef int (*NpAgent_createFromFd_fn)(int32_t fd, size_t size, NpAgentOptions* options);
typedef int (*NpAgent_createFromCacheBuffer_fn)(const void* buffer, size_t size,
                                                NpAgentOptions* options);
typedef int (*NpAgent_createFromCacheFd_fn)(int32_t fd, size_t size, NpAgentOptions* options);
typedef void (*NpAgent_release_fn)(int agentId);
typedef int (*NpAgent_getCompiledModelSize_fn)(int agentId, size_t* size);
typedef int (*NpAgent_storeCompiledModel_fn)(int agentId, void* buffer, size_t size);
typedef int (*NpAgentAttributes_create_fn)(int agentId, NpAgentAttributes** attributes);
typedef void (*NpAgentAttributes_release_fn)(NpAgentAttributes* attributes);
typedef int (*NpAgentAttributes_getInputFormat_fn)(NpAgentAttributes* attributes, uint32_t* format);
typedef int (*NpAgentAttributes_getOutputFormat_fn)(NpAgentAttributes* attributes,
                                                    uint32_t* format);
typedef int (*NpAgentAttributes_getInputCompressionMode_fn)(NpAgentAttributes* attributes,
                                                            uint32_t* mode);
typedef int (*NpAgentAttributes_getOutputCompressionMode_fn)(NpAgentAttributes* attributes,
                                                             uint32_t* mode);
typedef int (*NpAgentAttributes_getInputHeightWidth_fn)(NpAgentAttributes* attributes,
                                                        uint32_t* height, uint32_t* width);
typedef int (*NpAgentAttributes_getOutputHeightWidth_fn)(NpAgentAttributes* attributes,
                                                         uint32_t* height, uint32_t* width);
typedef int (*NpAgentAttributes_getInputStride_fn)(NpAgentAttributes* attributes, uint32_t* stride);
typedef int (*NpAgentAttributes_getOutputStride_fn)(NpAgentAttributes* attributes,
                                                    uint32_t* stride);
typedef int (*NpAgentExecution_create_fn)(NpAgentExecution** execution);
typedef void (*NpAgentExecution_release_fn)(NpAgentExecution* execution);
typedef int (*NpAgentExecution_setInput_fn)(NpAgentExecution* execution, int fd, size_t size);
typedef int (*NpAgentExecution_setOutput_fn)(NpAgentExecution* execution, int fd, size_t size);
typedef int (*NpAgent_compute_fn)(int agentId, NpAgentExecution* execution);
typedef int (*NpAgent_computeWithFence_fn)(int agentId, NpAgentExecution* execution, int inputFence,
                                           int outputFence, int* releaseFence, uint64_t duration);
typedef int (*NpAgent_validateInput_fn)(int agentId, uint32_t format, uint32_t height,
                                        uint32_t width, uint32_t stride);
typedef int (*NpAgent_validateOutput_fn)(int agentId, uint32_t format, uint32_t height,
                                         uint32_t width, uint32_t stride);
typedef int (*NpAgent_validateInputHeightWidth_fn)(int agentId, uint32_t height, uint32_t width);
typedef int (*NpAgent_validateOutputHeightWidth_fn)(int agentId, uint32_t height, uint32_t width);
typedef int (*NpAgent_updateOptions_fn)(int agentId, NpAgentOptions* options, uint32_t optionCode);
typedef int (*NpAgent_gpuCreate_fn)(const buffer_handle_t& handle);
typedef int (*NpAgent_gpuUpdate_fn)(int agentId, const buffer_handle_t& handle,
                                    uint32_t optionCode);
/*************************************************************************************************/

static void* sNpAgentHandle;
inline void* loadNpAgentLibrary(const char* name) {
    sNpAgentHandle = dlopen(name, RTLD_LAZY | RTLD_LOCAL);
    if (sNpAgentHandle == nullptr) {
        NPAGENT_LOG_E("NpAgent error: unable to open library %s", name);
    } else {
        NPAGENT_LOG_D("NpAgent : open library %s", name);
    }
    return sNpAgentHandle;
}

inline void* getNpAgentLibraryHandle() {
    if (sNpAgentHandle == nullptr) {
        if (sNpAgentHandle == nullptr) {
            sNpAgentHandle = loadNpAgentLibrary("libnpagent.so");
        }
    }

    return sNpAgentHandle;
}

inline void* loadNpAgentFunction(const char* name) {
    void* fn = nullptr;
    if (getNpAgentLibraryHandle() != nullptr) {
        fn = dlsym(getNpAgentLibraryHandle(), name);
    }

    if (fn == nullptr) {
        NPAGENT_LOG_E("NpAgent error: unable to open function %s", name);
    }

    return fn;
}

/**
 * @brief Create an {@link NpAgentOptions}.
 *
 * @param options The {@link NpAgentOptions} to be created. Set to NULL if unsuccessful.
 * @return RESULT_NO_ERROR if successful.
 */
inline int NpAgentOptions_create(NpAgentOptions** options) {
    LOAD_NPAGENT_FUNCTION(NpAgentOptions_create);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(options);
}

/**
 * @brief Destroy an {@link NpAgentOptions}.
 *
 * @param options The options to be destroyed.
 */
inline void NpAgentOptions_release(NpAgentOptions* options) {
    LOAD_NPAGENT_FUNCTION(NpAgentOptions_release);
    EXECUTE_NPAGENT_FUNCTION(options);
}

/**
 * @brief Set the size of the Agent pool. In this pool, there will be multiple Agents sharing the
 * same Agent ID to provide inference services.
 *
 * @param options The options to be configured.
 * @param size The size of the Agent pool.
 * @return RESULT_NO_ERROR if successful.
 */
inline int NpAgentOptions_setPoolSize(NpAgentOptions* options, uint32_t size) {
    LOAD_NPAGENT_FUNCTION(NpAgentOptions_setPoolSize);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(options, size);
}

/**
 * @brief Set the boost value of the Agent.
 *
 * @param options The options to be configured.
 * @value The boost value.
 * @return RESULT_NO_ERROR if successful.
 */
inline int NpAgentOptions_setBoostValue(NpAgentOptions* options, uint32_t value) {
    LOAD_NPAGENT_FUNCTION(NpAgentOptions_setBoostValue);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(options, value);
}

/**
 * @brief Set the input buffer format of this model.
 *
 * @param options The options to be configured.
 * @return RESULT_NO_ERROR if successful.
 */
inline int NpAgentOptions_setInputFormat(NpAgentOptions* options, uint32_t format) {
    LOAD_NPAGENT_FUNCTION(NpAgentOptions_setInputFormat);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(options, format);
}

/**
 * @brief Set the output buffer format of this model.
 *
 * @param options The options to be configured.
 * @return RESULT_NO_ERROR if successful.
 */
inline int NpAgentOptions_setOutputFormat(NpAgentOptions* options, uint32_t format) {
    LOAD_NPAGENT_FUNCTION(NpAgentOptions_setOutputFormat);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(options, format);
}

/**
 * @brief Set the inptu buffer compression mode of this model.
 *
 * @param options The options to be configured.
 * @return RESULT_NO_ERROR if successful.
 */
inline int NpAgentOptions_setInputCompressionMode(NpAgentOptions* options, uint32_t mode) {
    LOAD_NPAGENT_FUNCTION(NpAgentOptions_setInputCompressionMode);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(options, mode);
}

/**
 * @brief Set the output buffer compression mode of this model.
 *
 * @param options The options to be configured.
 * @return RESULT_NO_ERROR if successful.
 */
inline int NpAgentOptions_setOutputCompressionMode(NpAgentOptions* options, uint32_t mode) {
    LOAD_NPAGENT_FUNCTION(NpAgentOptions_setOutputCompressionMode);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(options, mode);
}

/**
 * @brief Set the input buffer height and width.
 *
 * @param options The options to be configured.
 * @param height Height value.
 * @param width Width value.
 * @return RESULT_NO_ERROR if successful.
 */
inline int NpAgentOptions_setInputHeightWidth(NpAgentOptions* options, uint32_t height,
                                              uint32_t width) {
    LOAD_NPAGENT_FUNCTION(NpAgentOptions_setInputHeightWidth);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(options, height, width);
}

/**
 * @brief Set the output buffer height and width.
 *
 * @param options The options to be configured.
 * @param height Height value.
 * @param width Width value.
 * @return RESULT_NO_ERROR if successful.
 */
inline int NpAgentOptions_setOutputHeightWidth(NpAgentOptions* options, uint32_t height,
                                               uint32_t width) {
    LOAD_NPAGENT_FUNCTION(NpAgentOptions_setOutputHeightWidth);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(options, height, width);
}

/**
 * @brief Set the input buffer stride.
 *
 * @param options The options to be configured.
 * @return RESULT_NO_ERROR if successful.
 */
inline int NpAgentOptions_setInputStride(NpAgentOptions* options, uint32_t stride) {
    LOAD_NPAGENT_FUNCTION(NpAgentOptions_setInputStride);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(options, stride);
}

/**
 * @brief Set the output buffer stride.
 *
 * @param options The options to be configured.
 * @return RESULT_NO_ERROR if successful.
 */
inline int NpAgentOptions_setOutputStride(NpAgentOptions* options, uint32_t stride) {
    LOAD_NPAGENT_FUNCTION(NpAgentOptions_setOutputStride);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(options, stride);
}

/**
 * @brief Create a NeuroPilot agent from TFLite model.
 *
 * @param buffer A pointer to the TFlite model to use.
 * @param size The size in bytes of the TFlite model.
 * @param options The options for compiling the TFLite model.
 * @return A positive id for the TFLite model if successful.
 */
inline int NpAgent_createFromBuffer(const void* buffer, size_t size, NpAgentOptions* options) {
    LOAD_NPAGENT_FUNCTION(NpAgent_createFromBuffer);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(buffer, size, options);
}

/**
 * @brief Create a NeuroPilot agent from TFLite model.
 *
 * @param fd FD of the TFlite model.
 * @param size The size in bytes of the TFlite model.
 * @param options The options for compiling the TFLite model.
 * @return A positive id for the TFLite model if successful.
 */
inline int NpAgent_createFromFd(int32_t fd, size_t size, NpAgentOptions* options) {
    LOAD_NPAGENT_FUNCTION(NpAgent_createFromFd);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(fd, size, options);
}

/**
 * @brief Create a NeuroPilot agent from the compiled model cache.
 *
 * @param buffer A pointer to the model cache.
 * @param size The size in bytes of the model cache.
 * @param options The options for loading the model cache.
 * @return A positive number means that the model cache was successfully loaded.
 */
inline int NpAgent_createFromCacheBuffer(const void* buffer, size_t size, NpAgentOptions* options) {
    LOAD_NPAGENT_FUNCTION(NpAgent_createFromCacheBuffer);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(buffer, size, options);
}

/**
 * @brief Create a NeuroPilot agent from the compiled model cache.
 *
 * @param fd  Hardwarebuffer fd of the model cache.
 * @param size size The size in bytes of the model cache.
 * @param options The options for loading the model cache.
 * @return A positive number means that the model cache was successfully loaded.
 */
inline int NpAgent_createFromCacheFd(int32_t fd, size_t size, NpAgentOptions* options) {
    LOAD_NPAGENT_FUNCTION(NpAgent_createFromCacheFd);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(fd, size, options);
}

/*
 * @brief Release the NeuroPilot agent.
 *
 * @param agentId NeuroPilot agent id to be destroyed.
 */
inline void NpAgent_release(int agentId) {
    LOAD_NPAGENT_FUNCTION(NpAgent_release);
    EXECUTE_NPAGENT_FUNCTION(agentId);
}

/*
 * @brief Get the size of the compiled model.
 *
 * @param agentId The id returned by {@link NpAgent_create} or {@link NpAgent_createFromCache}.
 * @param size The compiled network size in bytes.
 * @return RESULT_NO_ERROR if successful.
 */
inline int NpAgent_getCompiledModelSize(int agentId, size_t* size) {
    LOAD_NPAGENT_FUNCTION(NpAgent_getCompiledModelSize);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(agentId, size);
}

/*
 * @brief Get the size of the compiled model.
 *
 * @param agentId The id returned by {@link NpAgent_create} or {@link NpAgent_createFromCache}.
 * @param buffer User allocated buffer to store the compiled network.
 * @param size Size of the user allocated buffer in bytes.
 * @return RESULT_NO_ERROR if successful.
 */
inline int NpAgent_storeCompiledModel(int agentId, void* buffer, size_t size) {
    LOAD_NPAGENT_FUNCTION(NpAgent_storeCompiledModel);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(agentId, buffer, size);
}

/*
 * @brief Create a {@link NpAgentAttributes} to query model information.
 *
 * @param agentId NeuroPilot agent id to be queried.
 * @param attributes The {@link NpAgentAttributes} to store the information. Set to NULL if
 * unsuccessful. User needs to release the attributes by calling NpAgentAttributes_release.
 * @return RESULT_NO_ERROR if successful.
 */
inline int NpAgentAttributes_create(int agentId, NpAgentAttributes** attributes) {
    LOAD_NPAGENT_FUNCTION(NpAgentAttributes_create);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(agentId, attributes);
}

/**
 * @brief Destroy an {@link NpAgentAttributes}.
 *
 * @param attributes The attributes to be destroyed.
 */
inline void NpAgentAttributes_release(NpAgentAttributes* attributes) {
    LOAD_NPAGENT_FUNCTION(NpAgentAttributes_release);
    EXECUTE_NPAGENT_FUNCTION(attributes);
}

/*
 * @brief Get the input buffer format of this model.
 *
 * @param attributes The options to be queried.
 * @param The buffer format of this model
 * @return RESULT_NO_ERROR if successful.
 */
inline int NpAgentAttributes_getInputFormat(NpAgentAttributes* attributes, uint32_t* format) {
    LOAD_NPAGENT_FUNCTION(NpAgentAttributes_getInputFormat);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(attributes, format);
}

/*
 * @brief Get the output buffer format of this model.
 *
 * @param attributes The options to be queried.
 * @param The buffer format of this model
 * @return RESULT_NO_ERROR if successful.
 */
inline int NpAgentAttributes_getOutputFormat(NpAgentAttributes* attributes, uint32_t* format) {
    LOAD_NPAGENT_FUNCTION(NpAgentAttributes_getOutputFormat);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(attributes, format);
}

/*
 * @brief Get the input buffer compression mode of this model.
 *
 * @param attributes The attributes to be queried.
 * @param mode The buffer compression mode.
 * @return RESULT_NO_ERROR if successful.
 */
inline int NpAgentAttributes_getInputCompressionMode(NpAgentAttributes* attributes,
                                                     uint32_t* mode) {
    LOAD_NPAGENT_FUNCTION(NpAgentAttributes_getInputCompressionMode);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(attributes, mode);
}

/*
 * @brief Get the output buffer compression mode of this model.
 *
 * @param attributes The attributes to be queried.
 * @param mode The buffer compression mode.
 * @return RESULT_NO_ERROR if successful.
 */
inline int NpAgentAttributes_getOutputCompressionMode(NpAgentAttributes* attributes,
                                                      uint32_t* mode) {
    LOAD_NPAGENT_FUNCTION(NpAgentAttributes_getOutputCompressionMode);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(attributes, mode);
}

/*
 * @brief Get the input buffer height and width of this model.
 *
 * @param attributes The attributes to be queried.
 * @param height The input buffer height.
 * @param width The input buffer width.
 * @return RESULT_NO_ERROR if successful.
 */
inline int NpAgentAttributes_getInputHeightWidth(NpAgentAttributes* attributes, uint32_t* height,
                                                 uint32_t* width) {
    LOAD_NPAGENT_FUNCTION(NpAgentAttributes_getInputHeightWidth);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(attributes, height, width);
}

/*
 * @brief Get the ouptut buffer height and width of this model.
 *
 * @param attributes The attributes to be queried.
 * @param height The input buffer height.
 * @param width The input buffer width.
 * @return RESULT_NO_ERROR if successful.
 */
inline int NpAgentAttributes_getOutputHeightWidth(NpAgentAttributes* attributes, uint32_t* height,
                                                  uint32_t* width) {
    LOAD_NPAGENT_FUNCTION(NpAgentAttributes_getOutputHeightWidth);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(attributes, height, width);
}

/*
 * @brief Get the input buffer stride of this model.
 *
 * @param attributes The attributes to be queried.
 * @param stride The input buffer stride.
 * @return RESULT_NO_ERROR if successful.
 */
inline int NpAgentAttributes_getInputStride(NpAgentAttributes* attributes, uint32_t* stride) {
    LOAD_NPAGENT_FUNCTION(NpAgentAttributes_getInputStride);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(attributes, stride);
}

/*
 * @brief Get the buffer compression mode of this model.
 *
 * @param attributes The attributes to be queried.
 * @param stride The output buffer stride.
 * @return RESULT_NO_ERROR if successful.
 */
inline int NpAgentAttributes_getOutputStride(NpAgentAttributes* attributes, uint32_t* stride) {
    LOAD_NPAGENT_FUNCTION(NpAgentAttributes_getOutputStride);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(attributes, stride);
}

/*
 * @brief Create a fenced execution handle.
 *
 * @param input Fenced input to hold the input buffer and fence.
 * @return RESULT_NO_ERROR if successful.
 */
inline int NpAgentExecution_create(NpAgentExecution** execution) {
    LOAD_NPAGENT_FUNCTION(NpAgentExecution_create);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(execution);
}

/*
 * @brief Release the execution.
 *
 * @param execution The execution handle to be destroyed.
 *
 * @return RESULT_NO_ERROR if successful.
 */
inline void NpAgentExecution_release(NpAgentExecution* execution) {
    LOAD_NPAGENT_FUNCTION(NpAgentExecution_release);
    EXECUTE_NPAGENT_FUNCTION(execution);
}

/*
 * @brief Set the input buffer with a signal fence for execution. The input buffer contains AI model
 * and the execution input. User needs to map the input data into a Hardwarebuffer fd and signal the
 * fence once the execution input is ready to use.
 *
 * @param execution The execution handle.
 * @param fd Hardwarebuffer fd to hold the input buffer.
 * @param size The size in bytes of the input buffer.
 * @return RESULT_NO_ERROR if successful.
 */
inline int NpAgentExecution_setInput(NpAgentExecution* execution, int fd, size_t size) {
    LOAD_NPAGENT_FUNCTION(NpAgentExecution_setInput);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(execution, fd, size);
}

/*
 * @brief Set the output buffer for execution. User needs to map the output buffer into a
 * Hardwarebuffer fd.
 *
 * @param execution The execution handle.
 * @param fd Hardwarebuffer fd to hold the output buffer.
 * @param size The size in bytes of the output buffer.
 * @return RESULT_NO_ERROR if successful.
 */
inline int NpAgentExecution_setOutput(NpAgentExecution* execution, int fd, size_t size) {
    LOAD_NPAGENT_FUNCTION(NpAgentExecution_setOutput);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(execution, fd, size);
}

/*
 * @brief Invoke the fenced execution by the given {@NpAgentExecution}.
 *
 * @param agentId The id returned by {@link NpAgent_create} or {@link NpAgent_createFromCache}.
 * @param execution The execution after invoking {@link NpAgentExecution_setInput} and {@link
 * NpAgentExecution_setOutput}
 *
 * @return RESULT_NO_ERROR if successful.
 */
inline int NpAgent_compute(int agentId, NpAgentExecution* execution) {
    LOAD_NPAGENT_FUNCTION(NpAgent_compute);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(agentId, execution);
}

/*
 * @brief Invoke the fenced execution by the given {@NpAgentExecution}.
 *
 * @param agentId The id returned by {@link NpAgent_create} or {@link NpAgent_createFromCache}.
 * @param execution The execution after invoking {@link NpAgentExecution_setInput} and {@link
 * NpAgentExecution_setOutput}
 * @param inputFence Fence fd to indicate the readiness of the input data.
 * @param outputFence Fence fd to indicate the readiness of the output data.
 * @param releaseFence Fence fd to indicate the completeness of the computation
 * @param duration Set the maximum expected duration of the specified execution. If set to 0, the
 * timeout duration is considered infinite.
 *
 * @return RESULT_NO_ERROR if successful.
 */
inline int NpAgent_computeWithFence(int agentId, NpAgentExecution* execution, int inputFence,
                                    int outputFence, int* releaseFence, uint64_t duration) {
    LOAD_NPAGENT_FUNCTION(NpAgent_computeWithFence);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(agentId, execution, inputFence, outputFence, releaseFence,
                                        duration);
}

/**
 * @brief Check if the input buffer attributes are valid with the model represented by the agent ID.
 *
 * @param agentId The id returned by {@link NpAgent_create} or {@link NpAgent_createFromCache}.
 * @param format Input buffer format in {@link BufferFormat}.
 * @param height Input buffer height
 * @param width Input buffer width
 * @param stride Stride value
 * @return RESULT_NO_ERROR if the input buffer attributes are valid
 */
inline int NpAgent_validateInput(int agentId, uint32_t format, uint32_t height, uint32_t width,
                                 uint32_t stride) {
    LOAD_NPAGENT_FUNCTION(NpAgent_validateInput);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(agentId, format, height, width, stride);
}

/**
 * @brief Check if the output buffer attributes are valid with the model represented by the agent
 * ID.
 *
 * @param agentId The id returned by {@link NpAgent_create} or {@link NpAgent_createFromCache}.
 * @param format Output buffer format in {@link BufferFormat}.
 * @param height Output buffer height
 * @param width Output buffer width
 * @param stride Stride value
 * @return RESULT_NO_ERROR if the output buffer attributes are valid
 */
inline int NpAgent_validateOutput(int agentId, uint32_t format, uint32_t height, uint32_t width,
                                  uint32_t stride) {
    LOAD_NPAGENT_FUNCTION(NpAgent_validateOutput);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(agentId, format, height, width, stride);
}

/**
 * @brief Check if the input buffer height, width are valid with the model represented by the agent
 * ID.
 *
 * @param agentId The id returned by {@link NpAgent_create} or {@link NpAgent_createFromCache}.
 * @param height Output buffer height
 * @param width Output buffer width
 * @return RESULT_NO_ERROR if the output buffer attributes are valid
 */
inline int NpAgent_validateInputHeightWidth(int agentId, uint32_t height, uint32_t width) {
    LOAD_NPAGENT_FUNCTION(NpAgent_validateInputHeightWidth);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(agentId, height, width);
}

/**
 * @brief Check if the output buffer height, width are valid with the model represented by the agent
 * ID.
 *
 * @param agentId The id returned by {@link NpAgent_create} or {@link NpAgent_createFromCache}.
 * @param height Output buffer height
 * @param width Output buffer width
 * @return RESULT_NO_ERROR if the output buffer attributes are valid
 */
inline int NpAgent_validateOutputHeightWidth(int agentId, uint32_t height, uint32_t width) {
    LOAD_NPAGENT_FUNCTION(NpAgent_validateOutputHeightWidth);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(agentId, height, width);
}

/*
 * @brief Update the options of a created NeuroPilot agent.
 *
 * @param agentId The id returned by {@link NpAgent_create} or {@link NpAgent_createFromCache}.
 * @param options The options to be updated.
 * @param optionCode User specified options to be updated. Must be one of OPTION_* or the inclusive
 * OR value of multiple OPTION_*.
 * @return RESULT_NO_ERROR if successful.
 */
inline int NpAgent_updateOptions(int agentId, NpAgentOptions* options, uint32_t optionCode) {
    LOAD_NPAGENT_FUNCTION(NpAgent_updateOptions);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(agentId, options, optionCode);
}

/**
 * @brief Create an {@link NpAgent} for GPU scenario.
 *
 * @param handle A buffer handle containing the metadata for creating the NpAgent.
 * @return A positive number means that the agent cache was successfully created.
 */
inline int NpAgent_gpuCreate(const buffer_handle_t& handle) {
    LOAD_NPAGENT_FUNCTION(NpAgent_gpuCreate);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(handle);
}

/**
 * @brief Update the options of the created NpAgent for GPU scenario.
 *
 * @param agentId The id returned by {@link NpAgent_gpuCreate}.
 * @param handle A buffer handle containing the metadata for creating the NpAgent.
 * @return A positive number means that the agent cache was successfully created.
 */
inline int NpAgent_gpuUpdate(int agentId, const buffer_handle_t& handle, uint32_t optionCode) {
    LOAD_NPAGENT_FUNCTION(NpAgent_gpuUpdate);
    EXECUTE_NPAGENT_FUNCTION_RETURN_INT(agentId, handle, optionCode);
}

#endif  // __NEUROPILOTAGENT_SHIM_H__
