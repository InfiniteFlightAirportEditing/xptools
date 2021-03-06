/*
 * Copyright (c) 2007, Laminar Research.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef WED_ATCFrequency_H
#define WED_ATCFrequency_H

#include "WED_Thing.h"

struct	AptATCFreq_t;

class WED_ATCFrequency : public WED_Thing {

DECLARE_PERSISTENT(WED_ATCFrequency)

public:

	void	Import(const AptATCFreq_t& info, void (* print_func)(void *, const char *, ...), void * ref);
	void	Export(		 AptATCFreq_t& info) const;

	virtual const char *	HumanReadableType(void) const { return "ATC Frequency"; }

private:

	WED_PropIntEnum		freq_type;
	WED_PropDoubleText	freq;

};

#endif /* WED_ATCFrequency_H */
