/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef BACKTRACE_SUPPORTED
//#define BACKWARD_HAS_BFD 1
#define BACKWARD_HAS_DW 1
//#define BACKWARD_HAS_DWARF 1
#endif

#include "stacktrace.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef BACKTRACE_SUPPORTED
#include <backward.hpp>
#endif
#include <execinfo.h>
#include <stdio.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace stacktrace {

    void stacktrace([[maybe_unused]] std::size_t trace_cnt_max) {
#ifdef BACKTRACE_SUPPORTED
        void* buffer[2];
        backtrace(buffer, 2);

        for (int i = 0; i < 2; i++) {
            std::cout << i << ": " << buffer[i] << std::endl;
        }

        backward::StackTrace st;
        //        st.load_here(trace_cnt_max);
        st.load_from(reinterpret_cast<char*>(buffer[1]) - 1, trace_cnt_max);

        backward::Printer p;
        p.snippet = true;
        p.object = false;
        p.color_mode = backward::ColorMode::automatic;
        p.address = true;
        p.print(st, stderr);
#endif
    }
} // namespace stacktrace

/*
#include <execinfo.h>
#include <stdio.h>
...
void* callstack[128];
int i, frames = backtrace(callstack, 128);
char** strs = backtrace_symbols(callstack, frames);
for (i = 0; i < frames; ++i) {
    printf("%s\n", strs[i]);
}
free(strs);
...
 */

/*
class MyException : public std::exception {

    char ** strs;
    MyException( const std::string & message ) {
         int i, frames = backtrace(callstack, 128);
         strs = backtrace_symbols(callstack, frames);
    }

    void printStackTrace() {
        for (i = 0; i < frames; ++i) {
            printf("%s\n", strs[i]);
        }
        free(strs);
    }
};
 */
/*
try {
   throw MyException("Oops!");
} catch ( MyException e ) {
    e.printStackTrace();
}
 */
