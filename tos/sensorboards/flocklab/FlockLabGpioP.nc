// $Id: FlockLabGpioP.nc 2375 2013-04-19 08:35:27Z walserc $
/**
 * @author Roman Lim <lim@tik.ee.ethz.ch>
 *
 * @section LICENSE
 *
 * Copyright (c) 2013 ETH Zurich.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, LOSS OF USE, DATA,
 * OR PROFITS) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

module FlockLabGpioP {
	uses {
		interface GeneralIO as SIG1;
		interface GeneralIO as SIG2;
		interface GeneralIO as FLOCKLAB_INT1;
		interface GeneralIO as FLOCKLAB_INT2;
	}
	provides interface Init;

}
implementation {
	
	command error_t Init.init() {
		call SIG1.makeInput();
		call SIG2.makeInput();
		call FLOCKLAB_INT1.clr();
		call FLOCKLAB_INT1.makeOutput();
		call FLOCKLAB_INT2.clr();
		call FLOCKLAB_INT2.makeOutput();
		return SUCCESS;
	}
	
}
