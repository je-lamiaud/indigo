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

var theMount = {devName: null, rules: []};
var theJoystick = {devName: null, rules: []};
var theGPS = {devName: null, rules: []};

const log_prefix = "Mount snoop: ";
const Mount_Mask    = 0x00001;
const Joystick_Mask = 0x18000;
const GPS_Mask      = 0x00040;
const Agent_Mask    = 0x04000;

indigo_log(log_prefix + "Starting automatic mount snoop handling");

function addSnoop(device)
{
   if (device.devName != null && theMount.devName != null)
   {
      var snoop = indigo_devices["Snoop Agent"];
      var mount = indigo_devices[theMount.devName];
      // Connect properties with identical names
      for (var devProp in indigo_devices[device.devName])
      {
         if ((devProp.startsWith("MOUNT") || devProp.startsWith("GEOGRAPHIC") || devProp.startsWith("UTC"))
             && mount[devProp])
         {
            rule = {SOURCE_DEVICE: device.devName, SOURCE_PROPERTY: devProp,
                    TARGET_DEVICE: theMount.devName, TARGET_PROPERTY: devProp};
            indigo_log(log_prefix + "Connecting "
                                    + rule.SOURCE_DEVICE + "." + rule.SOURCE_PROPERTY
                                    + " to " + rule.TARGET_DEVICE + "." + rule.TARGET_PROPERTY);
            snoop.SNOOP_ADD_RULE.change(rule);
            device.rules.push(rule);
         }
      }
   }
}

function delSnoop(device)
{
   var snoop = indigo_devices["Snoop Agent"];
   while (device.rules.length != 0)
   {
      var r = device.rules.pop();
      indigo_log(log_prefix + "Disconnecting "
                             + r.SOURCE_DEVICE + "." + r.SOURCE_PROPERTY
                             + " from " + r.TARGET_DEVICE + "." + r.TARGET_PROPERTY);
      snoop.SNOOP_REMOVE_RULE.change(r);
   }
}

function loadConfig(device)
{
   indigo_devices[device.devName].CONFIG.change({LOAD: true});
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
            if (theMount.devName == dev_name)
            {
               indigo_log(log_prefix + "Mount " + dev_name + " removed, disconnected");
               delSnoop(theJoystick);
               delSnoop(theGPS);
               theMount.devName = null;
            }
            else if (theJoystick.devName == dev_name)
            {
               indigo_log(log_prefix + "Joystick " + dev_name + " removed, disconnected");
               delSnoop(theJoystick);
               theJoystick.devName = null;
            }
            else if (theGPS.devName == dev_name)
            {
               indigo_log(log_prefix + "GPS " + dev_name + " removed, disconnected");
               delSnoop(theGPS);
               theGPS.devName = null;
            }
         }
         else if (property.items.CONNECTED)
         {
            var itf = dev.INFO.items.DEVICE_INTERFACE;
            if (itf != undefined)
            {
               if ((itf & (Mount_Mask | Agent_Mask)) == Mount_Mask
                   && theMount.devName == null)
               {
                  indigo_log(log_prefix + "Mount set to " + dev_name);
                  theMount.devName = dev_name;
                  loadConfig(theMount);
                  addSnoop(theJoystick)
                  addSnoop(theGPS)
               }
               else if ((itf & (Joystick_Mask | Agent_Mask)) == Joystick_Mask
                        && theJoystick.devName == null)
               {
                  indigo_log(log_prefix + "Joystick set to " + dev_name);
                  theJoystick.devName = dev_name;
                  loadConfig(theJoystick);
                  addSnoop(theJoystick)
               }
               else if ((itf & (GPS_Mask | Agent_Mask)) == GPS_Mask
                        && theGPS.devName == null)
               {
                  indigo_log(log_prefix + "GPS set to " + dev_name);
                  theGPS.devName = dev_name;
                  loadConfig(theGPS);
                  addSnoop(theGPS)
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
         if (theMount.devName == dev_name)
         {
            indigo_log(log_prefix + "Mount " + dev_name + " removed, deleted");
            delSnoop(theJoystick);
            delSnoop(theGPS);
            theMount.devName = null;
         }
         if (theJoystick.devName == dev_name)
         {
            indigo_log(log_prefix + "Joystick " + dev_name + " removed, deleted");
            delSnoop(theJoystick)
            theJoystick.devName = null;
         }
         if (theGPS.devName != null && theGPS == dev_name)
         {
            indigo_log(log_prefix + "GPS " + dev_name + " removed, deleted");
            delSnoop(theGPS)
            theGPS.devName = null;
         }
      }
   }
};
