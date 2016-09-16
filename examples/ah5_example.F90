!*******************************************************************************
! Copyright (c) 2013-2014, Julien Bigot - CEA (julien.bigot@cea.fr)
! All rights reserved.
!
! Redistribution and use in source and binary forms, with or without
! modification, are permitted provided that the following conditions are met:
!     * Redistributions of source code must retain the above copyright
!     notice, this list of conditions and the following disclaimer.
!     * Redistributions in binary form must reproduce the above copyright
!     notice, this list of conditions and the following disclaimer in the
!     documentation and/or other materials provided with the distribution.
!     * Neither the name of the <organization> nor the
!     names of its contributors may be used to endorse or promote products
!     derived from this software without specific prior written permission.
!
! THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
! IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
! FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
! AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
! LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
! OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
! THE SOFTWARE.
!*******************************************************************************

include 'Ah5.f90'

program ah5_example

use ah5

integer, parameter :: DATA_HEIGHT = 2048
integer, parameter :: DATA_WIDTH = 512

type(ah5_t) :: ah5_inst

real(8), pointer :: data(:,:), data_next(:,:), data_swap(:,:)
integer :: ii, err
character(15) :: fname

allocate(data(DATA_WIDTH,DATA_HEIGHT))
call data_init(data)
call ah5_init(ah5_inst, err)
allocate(data_next(DATA_WIDTH,DATA_HEIGHT))

do ii = 0, 100
  if ( mod(ii, 10) == 0 ) then
    write (fname, '("data.",I4.4,".h5")') ii
    call ah5_open(ah5_inst, fname, err)
    call ah5_write(ah5_inst, data, "data", err)
    call ah5_close(ah5_inst, err)
  endif

  call data_compute(data, data_next)
  data_swap => data
  data => data_next
  data_next => data_swap
enddo

write (fname, '("data.",I4.4,".h5")') ii
call ah5_open(ah5_inst, fname, err)
call ah5_write(ah5_inst, data, "data", err)
call ah5_close(ah5_inst, err)

call ah5_finalize(ah5_inst, err);

deallocate(data)
deallocate(data_next)

contains

subroutine data_init(data)

  real(8), intent(OUT) :: data(DATA_WIDTH,DATA_HEIGHT)

  integer :: xx, yy

  do xx = 1, DATA_WIDTH
    do yy = 1, DATA_HEIGHT
      data(xx,yy) = sin(1.*(xx-1.)/DATA_WIDTH)*sin(1.*(yy-1.)/DATA_HEIGHT)
    enddo
  enddo

endsubroutine data_init


subroutine data_compute(data_in, data_out)

  real(8), intent(IN) :: data_in(DATA_WIDTH,DATA_HEIGHT)
  real(8), intent(OUT) :: data_out(DATA_WIDTH,DATA_HEIGHT)

  integer :: xx, yy

  do xx = 1, DATA_WIDTH
    do yy = 1, DATA_HEIGHT
      data_out(xx,yy) = &
          0.5 * data_in(xx, yy) &
          + 0.125 * data_in(mod(xx+DATA_WIDTH-2,DATA_WIDTH)+1, yy) &
          + 0.125 * data_in(mod(xx, DATA_WIDTH)+1, yy) &
          + 0.125 * data_in(xx, mod(yy+DATA_HEIGHT-2,DATA_HEIGHT)+1) &
          + 0.125 * data_in(xx, mod(yy,DATA_HEIGHT)+1)
    enddo
  enddo

endsubroutine data_compute

endprogram ah5_example
