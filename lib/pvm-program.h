/* pvm-program.h - Internal header for PVM programs.  */

/* Copyright (C) 2020 Jose E. Marchesi */

/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PVM_PROGRAM_H
#define PVM_PROGRAM_H

#include "pvm-vm.h"

/* Get the jitter routine associated with the program PROGRAM.  */
pvm_routine pvm_program_routine (pvm_program program);

#endif /* ! PVM_PROGRAM_H */
