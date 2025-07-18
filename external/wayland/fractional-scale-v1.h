/* Generated by wayland-scanner 1.23.1 */

#ifndef FRACTIONAL_SCALE_V1_CLIENT_PROTOCOL_H
#define FRACTIONAL_SCALE_V1_CLIENT_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include "external/wayland/wayland-client.h"

#ifdef  __cplusplus
extern "C" {
#endif

/**
 * @page page_fractional_scale_v1 The fractional_scale_v1 protocol
 * Protocol for requesting fractional surface scales
 *
 * @section page_desc_fractional_scale_v1 Description
 *
 * This protocol allows a compositor to suggest for surfaces to render at
 * fractional scales.
 *
 * A client can submit scaled content by utilizing wp_viewport. This is done by
 * creating a wp_viewport object for the surface and setting the destination
 * rectangle to the surface size before the scale factor is applied.
 *
 * The buffer size is calculated by multiplying the surface size by the
 * intended scale.
 *
 * The wl_surface buffer scale should remain set to 1.
 *
 * If a surface has a surface-local size of 100 px by 50 px and wishes to
 * submit buffers with a scale of 1.5, then a buffer of 150px by 75 px should
 * be used and the wp_viewport destination rectangle should be 100 px by 50 px.
 *
 * For toplevel surfaces, the size is rounded halfway away from zero. The
 * rounding algorithm for subsurface position and size is not defined.
 *
 * @section page_ifaces_fractional_scale_v1 Interfaces
 * - @subpage page_iface_wp_fractional_scale_manager_v1 - fractional surface scale information
 * - @subpage page_iface_wp_fractional_scale_v1 - fractional scale interface to a wl_surface
 * @section page_copyright_fractional_scale_v1 Copyright
 * <pre>
 *
 * Copyright © 2022 Kenny Levinsen
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 * </pre>
 */
struct wl_surface;
struct wp_fractional_scale_manager_v1;
struct wp_fractional_scale_v1;

#ifndef WP_FRACTIONAL_SCALE_MANAGER_V1_INTERFACE
#define WP_FRACTIONAL_SCALE_MANAGER_V1_INTERFACE
/**
 * @page page_iface_wp_fractional_scale_manager_v1 wp_fractional_scale_manager_v1
 * @section page_iface_wp_fractional_scale_manager_v1_desc Description
 *
 * A global interface for requesting surfaces to use fractional scales.
 * @section page_iface_wp_fractional_scale_manager_v1_api API
 * See @ref iface_wp_fractional_scale_manager_v1.
 */
/**
 * @defgroup iface_wp_fractional_scale_manager_v1 The wp_fractional_scale_manager_v1 interface
 *
 * A global interface for requesting surfaces to use fractional scales.
 */
extern const struct wl_interface wp_fractional_scale_manager_v1_interface;
#endif
#ifndef WP_FRACTIONAL_SCALE_V1_INTERFACE
#define WP_FRACTIONAL_SCALE_V1_INTERFACE
/**
 * @page page_iface_wp_fractional_scale_v1 wp_fractional_scale_v1
 * @section page_iface_wp_fractional_scale_v1_desc Description
 *
 * An additional interface to a wl_surface object which allows the compositor
 * to inform the client of the preferred scale.
 * @section page_iface_wp_fractional_scale_v1_api API
 * See @ref iface_wp_fractional_scale_v1.
 */
/**
 * @defgroup iface_wp_fractional_scale_v1 The wp_fractional_scale_v1 interface
 *
 * An additional interface to a wl_surface object which allows the compositor
 * to inform the client of the preferred scale.
 */
extern const struct wl_interface wp_fractional_scale_v1_interface;
#endif

#ifndef WP_FRACTIONAL_SCALE_MANAGER_V1_ERROR_ENUM
#define WP_FRACTIONAL_SCALE_MANAGER_V1_ERROR_ENUM
enum wp_fractional_scale_manager_v1_error {
  /**
   * the surface already has a fractional_scale object associated
   */
  WP_FRACTIONAL_SCALE_MANAGER_V1_ERROR_FRACTIONAL_SCALE_EXISTS = 0,
};
#endif /* WP_FRACTIONAL_SCALE_MANAGER_V1_ERROR_ENUM */

#define WP_FRACTIONAL_SCALE_MANAGER_V1_DESTROY 0
#define WP_FRACTIONAL_SCALE_MANAGER_V1_GET_FRACTIONAL_SCALE 1


/**
 * @ingroup iface_wp_fractional_scale_manager_v1
 */
#define WP_FRACTIONAL_SCALE_MANAGER_V1_DESTROY_SINCE_VERSION 1
/**
 * @ingroup iface_wp_fractional_scale_manager_v1
 */
#define WP_FRACTIONAL_SCALE_MANAGER_V1_GET_FRACTIONAL_SCALE_SINCE_VERSION 1

/** @ingroup iface_wp_fractional_scale_manager_v1 */
static inline void
wp_fractional_scale_manager_v1_set_user_data(struct wp_fractional_scale_manager_v1 *wp_fractional_scale_manager_v1, void *user_data)
{
  wl_proxy_set_user_data((struct wl_proxy *) wp_fractional_scale_manager_v1, user_data);
}

/** @ingroup iface_wp_fractional_scale_manager_v1 */
static inline void *
wp_fractional_scale_manager_v1_get_user_data(struct wp_fractional_scale_manager_v1 *wp_fractional_scale_manager_v1)
{
  return wl_proxy_get_user_data((struct wl_proxy *) wp_fractional_scale_manager_v1);
}

static inline uint32_t
wp_fractional_scale_manager_v1_get_version(struct wp_fractional_scale_manager_v1 *wp_fractional_scale_manager_v1)
{
  return wl_proxy_get_version((struct wl_proxy *) wp_fractional_scale_manager_v1);
}

/**
 * @ingroup iface_wp_fractional_scale_manager_v1
 *
 * Informs the server that the client will not be using this protocol
 * object anymore. This does not affect any other objects,
 * wp_fractional_scale_v1 objects included.
 */
static inline void
wp_fractional_scale_manager_v1_destroy(struct wp_fractional_scale_manager_v1 *wp_fractional_scale_manager_v1)
{
  wl_proxy_marshal_flags((struct wl_proxy *) wp_fractional_scale_manager_v1,
       WP_FRACTIONAL_SCALE_MANAGER_V1_DESTROY, NULL, wl_proxy_get_version((struct wl_proxy *) wp_fractional_scale_manager_v1), WL_MARSHAL_FLAG_DESTROY);
}

/**
 * @ingroup iface_wp_fractional_scale_manager_v1
 *
 * Create an add-on object for the the wl_surface to let the compositor
 * request fractional scales. If the given wl_surface already has a
 * wp_fractional_scale_v1 object associated, the fractional_scale_exists
 * protocol error is raised.
 */
static inline struct wp_fractional_scale_v1 *
wp_fractional_scale_manager_v1_get_fractional_scale(struct wp_fractional_scale_manager_v1 *wp_fractional_scale_manager_v1, struct wl_surface *surface)
{
  struct wl_proxy *id;

  id = wl_proxy_marshal_flags((struct wl_proxy *) wp_fractional_scale_manager_v1,
       WP_FRACTIONAL_SCALE_MANAGER_V1_GET_FRACTIONAL_SCALE, &wp_fractional_scale_v1_interface, wl_proxy_get_version((struct wl_proxy *) wp_fractional_scale_manager_v1), 0, NULL, surface);

  return (struct wp_fractional_scale_v1 *) id;
}

/**
 * @ingroup iface_wp_fractional_scale_v1
 * @struct wp_fractional_scale_v1_listener
 */
struct wp_fractional_scale_v1_listener {
  /**
   * notify of new preferred scale
   *
   * Notification of a new preferred scale for this surface that
   * the compositor suggests that the client should use.
   *
   * The sent scale is the numerator of a fraction with a denominator
   * of 120.
   * @param scale the new preferred scale
   */
  void (*preferred_scale)(void *data,
        struct wp_fractional_scale_v1 *wp_fractional_scale_v1,
        uint32_t scale);
};

/**
 * @ingroup iface_wp_fractional_scale_v1
 */
static inline int
wp_fractional_scale_v1_add_listener(struct wp_fractional_scale_v1 *wp_fractional_scale_v1,
            const struct wp_fractional_scale_v1_listener *listener, void *data)
{
  return wl_proxy_add_listener((struct wl_proxy *) wp_fractional_scale_v1,
             (void (**)(void)) listener, data);
}

#define WP_FRACTIONAL_SCALE_V1_DESTROY 0

/**
 * @ingroup iface_wp_fractional_scale_v1
 */
#define WP_FRACTIONAL_SCALE_V1_PREFERRED_SCALE_SINCE_VERSION 1

/**
 * @ingroup iface_wp_fractional_scale_v1
 */
#define WP_FRACTIONAL_SCALE_V1_DESTROY_SINCE_VERSION 1

/** @ingroup iface_wp_fractional_scale_v1 */
static inline void
wp_fractional_scale_v1_set_user_data(struct wp_fractional_scale_v1 *wp_fractional_scale_v1, void *user_data)
{
  wl_proxy_set_user_data((struct wl_proxy *) wp_fractional_scale_v1, user_data);
}

/** @ingroup iface_wp_fractional_scale_v1 */
static inline void *
wp_fractional_scale_v1_get_user_data(struct wp_fractional_scale_v1 *wp_fractional_scale_v1)
{
  return wl_proxy_get_user_data((struct wl_proxy *) wp_fractional_scale_v1);
}

static inline uint32_t
wp_fractional_scale_v1_get_version(struct wp_fractional_scale_v1 *wp_fractional_scale_v1)
{
  return wl_proxy_get_version((struct wl_proxy *) wp_fractional_scale_v1);
}

/**
 * @ingroup iface_wp_fractional_scale_v1
 *
 * Destroy the fractional scale object. When this object is destroyed,
 * preferred_scale events will no longer be sent.
 */
static inline void
wp_fractional_scale_v1_destroy(struct wp_fractional_scale_v1 *wp_fractional_scale_v1)
{
  wl_proxy_marshal_flags((struct wl_proxy *) wp_fractional_scale_v1,
       WP_FRACTIONAL_SCALE_V1_DESTROY, NULL, wl_proxy_get_version((struct wl_proxy *) wp_fractional_scale_v1), WL_MARSHAL_FLAG_DESTROY);
}

#ifdef  __cplusplus
}
#endif

#endif
