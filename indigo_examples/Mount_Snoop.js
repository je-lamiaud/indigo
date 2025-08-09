// Copyright (c) 2025 J-E Lamiaud
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

var theMount = null;
var theJoystick = null;
var theGPS = null;

const log_prefix = "Mount snoop: ";
const Mount_Mask    = 0x00001;
const Joystick_Mask = 0x18000;
const GPS_Mask      = 0x00040;
const Agent_Mask    = 0x04000;

indigo_log(log_prefix + "Starting automatic mount snoop handling");

function setSnoop(item, device)
{
   var dev_name;
   if (device == null)
      dev_name = "";
   else
      dev_name = device;

   if (theMount != null)
   {
      var snoopProp = indigo_devices[theMount].SNOOP_DEVICES;
      var snoopItem = snoopProp.items[item];
      if (snoopItem != undefined && snoopItem != dev_name)
      {
         var items = {};
         items[item] = dev_name;
         indigo_devices[theMount].SNOOP_DEVICES.change(items);
      }
   }
}

function loadConfig(device)
{
   indigo_devices[device].CONFIG.change({LOAD: true});
}

indigo_event_handlers.Mount_snoop_handler = {
   devices: null,
   on_update: function(property) {
      if (property.name == 'CONNECTION' && property.state == "Ok")
      {
         var dev_name = property.device;
         var dev = indigo_devices[dev_name];
         // indigo_log(log_prefix + property.name + " update on " + dev_name);
         if (property.items.DISCONNECTED)
         {
            if (theMount == dev_name)
            {
               indigo_log(log_prefix + "Mount " + dev_name + " removed, disconnected");
               theMount = null;
            }
            else if (theJoystick == dev_name)
            {
               indigo_log(log_prefix + "Joystick " + dev_name + " removed, disconnected");
               theJoystick = null;
               setSnoop("JOYSTICK", theJoystick);
            }
            else if (theGPS == dev_name)
            {
               indigo_log(log_prefix + "GPS " + dev_name + " removed, disconnected");
               theGPS = null;
               setSnoop("GPS", theGPS);
            }
         }
         else if (property.items.CONNECTED)
         {
            var itf = dev.INFO.items.DEVICE_INTERFACE;
            if (itf != undefined)
            {
               if ((itf & (Mount_Mask | Agent_Mask)) == Mount_Mask
                   && theMount == null)
               {
                  indigo_log(log_prefix + "Mount set to" + dev_name);
                  theMount = dev_name;
                  loadConfig(theMount);
                  setSnoop("JOYSTICK", theJoystick)
                  setSnoop("GPS", theGPS)
               }
               else if ((itf & (Joystick_Mask | Agent_Mask)) == Joystick_Mask
                        && theJoystick == null)
               {
                  indigo_log(log_prefix + "Joystick set to" + dev_name);
                  theJoystick = dev_name;
                  loadConfig(theJoystick);
                  setSnoop("JOYSTICK", theJoystick)
               }
               else if ((itf & (GPS_Mask | Agent_Mask)) == GPS_Mask
                        && theGPS == null)
               {
                  indigo_log(log_prefix + "GPS set to" + dev_name);
                  theGPS = dev_name;
                  loadConfig(theGPS);
                  setSnoop("GPS", theGPS)
               }
            }
         }
      }
   },
   on_delete: function(property) {
      if (property.name == 'CONNECTION')
      {
         var dev_name = property.device;
         // indigo_log(log_prefix + property.name + " delete on " + dev_name);
         if (theMount == dev_name)
         {
            indigo_log(log_prefix + "Mount " + dev_name + " removed, deleted");
            theMount = null;
         }
         if (theJoystick == dev_name)
         {
            indigo_log(log_prefix + "Joystick " + dev_name + " removed, deleted");
            theJoystick = null;
            setSnoop("JOYSTICK", theJoystick)
         }
         if (theGPS == dev_name)
         {
            indigo_log(log_prefix + "GPS " + dev_name + " removed, deleted");
            theGPS = null;
            setSnoop("GPS", theGPS)
         }
      }
   }
};
