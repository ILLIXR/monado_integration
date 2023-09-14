// Copyright 2019, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header with only documentation.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_util
 */

#pragma once


/*!
 * @defgroup aux Auxiliary
 * @ingroup xrt
 *
 * @brief Shared code and helpers for Monado.
 */

/*!
 * @defgroup aux_util Utilities
 * @ingroup aux
 *
 * @brief Smaller pieces of auxiliary utilities code.
 */


/*!
 * @dir src/xrt/auxiliary
 * @ingroup xrt
 *
 * @brief Shared code and helpers for Monado.
 */

/*!
 * @dir src/xrt/auxiliary/util
 * @ingroup aux
 *
 * @brief Smaller pieces of auxiliary utilities code.
 */

#ifdef __cplusplus
/*!
 * @brief C++-only APIs in Monado.
 *
 * There are not very many of them.
 */
namespace xrt {

/*!
 * @brief C++-only functionality from assorted helper libraries
 */
namespace auxiliary {

	/*!
	 * @brief C++-only functionality from the miscellaneous "util" helper library
	 */
	namespace util {
		// Empty
	} // namespace util

} // namespace auxiliary

} // namespace xrt

#endif
