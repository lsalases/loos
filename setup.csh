# Setup environment for bourne-shell-like shells...
#
#  This file is part of LOOS.
#
#  LOOS (Lightweight Object-Oriented Structure library)
#  Copyright (c) 2012 Tod D. Romo, Grossfield Lab
#  Department of Biochemistry and Biophysics
#  School of Medicine & Dentistry, University of Rochester
#
#  This package (LOOS) is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation under version 3 of the License.
#
#  This package is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

###
### Set this to where LOOS is installed, or where the uninstalled distribution is...
###
set DEFAULT_PATH=""
###
###
###

if ( $DEFAULT_PATH == "" ) then
    set LOOSPATH="$cwd"
else
    set LOOSPATH="$DEFAULT_PATH"
endif

if ( -d $LOOSPATH/lib ) then
    set LIBPATH="$LOOSPATH/lib"
else
    set LIBPATH=$cwd
endif

if ( -d $LOOSPATH/bin ) then
    set TOOLPATH="$LOOSPATH/bin"
else
    set TOOLPATH="$LOOSPATH/Tools:$LOOSPATH/Packages/Convergence:$LOOSPATH/Packages/DensityTools:$LOOSPATH/Packages/ElasticNetworks:$LOOSPATH/Packages/HydrogenBonds:$LOOSPATH/Packages/Users"
endif

setenv PATH "$PATH":"$TOOLPATH"
setenv LD_LIBRARY_PATH "$LD_LIBRARY_PATH":"$LIBPATH"
