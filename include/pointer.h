/*
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */
#ifndef WLCS_POINTER_H_
#define WLCS_POINTER_H_

#include <wayland-client-core.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WlcsPointer WlcsPointer;

void wlcs_pointer_move_absolute(WlcsPointer* pointer, wl_fixed_t x, wl_fixed_t y) __attribute__((weak));
void wlcs_pointer_move_relative(WlcsPointer* pointer, wl_fixed_t dx, wl_fixed_t dy) __attribute__((weak));
void wlcs_pointer_button_up(WlcsPointer* pointer, int button) __attribute__((weak));
void wlcs_pointer_button_down(WlcsPointer* pointer, int button) __attribute__((weak));

#ifdef __cplusplus
}
#endif

#endif //WLCS_SERVER_H_
