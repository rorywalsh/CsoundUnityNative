### CsoundUnityNative

CsoundUnityNative wraps Csound in a native Unity audio plugin. It is intended to be used within Cabbage, but instructions are given below for its use outside of the cabbage export function.

Using CsoundUnityNative without Cabbage.
CsoundUnityNative is packaged as a single plugin library. When loaded, the plugin will read the current directory for a Csound file of the same name. If you wish to create bespoke plugins, simply copy and rename the CsoundUnityNative plugin to match the name of you .csd file. For example, if you create a .csd file called FunkyTown.csd. Copy the CsoundNativeUnity library and rename it to FunckyTown.dll, or FunkyTown.dylib. The next time Unity starts it will load your plugin. Note that the name of the plugin as it appears in Unity is determined by the caption passed to the form identifier. 

CsoudUnityNative is copyright (c) 2016 Rory Walsh.
CsoudUnityNative is free software; you can redistribute them and/or modify them under the terms of the GNU Lesser General Public License as published by the Free Software Foundation; either version 2.1 of the License, or (at your option) any later version.

CsoudUnityNative is distributed in the hope that they will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with this software; if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA