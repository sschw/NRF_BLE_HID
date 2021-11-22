#pragma once
/** @file
 *  @brief HoG Service sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void hog_init(void);
void hog_send_button(struct bt_conn *conn, uint8_t* inputReportData, uint16_t size);

#ifdef __cplusplus
}
#endif
