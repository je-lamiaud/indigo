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

var theCCD = [];
var theStation = null;

const log_prefix = "Weather_to_FITS: ";
const CCD_Mask     = 0x000002;
const Weather_Mask = 0x408000;
const Agent_Mask   = 0x004000;

indigo_log(log_prefix + "Starting reporting weather data in FITS header");

function loadConfig(device)
{
   indigo_devices[device].CONFIG.change({LOAD: true});
}

function setWeatherInFITS(property)
{
   if (property != null)
   {
      var Temperature = property.items.TEMPERATURE;
      var Humidity = property.items.HUMIDITY;
      // indigo_log(log_prefix + "Temperature " + Temperature + ", Humidity " + Humidity);

      for (idx in theCCD)
      {
         ccd_name = theCCD[idx];
         ccd = indigo_devices [ccd_name];
         if (ccd != null && ccd.CCD_SET_FITS_HEADER != null)
         {
            // indigo_log(log_prefix + "setting weather data in FITS of " + ccd_name);
            ccd.CCD_SET_FITS_HEADER.change ({KEYWORD: "AMBTEMP", VALUE: String(Temperature)});
            ccd.CCD_SET_FITS_HEADER.change ({KEYWORD: "HUMIDITY", VALUE: String(Humidity)});
         }
      }
   }
   else
   {
      // Remove the FITS keywords
      for (idx in theCCD)
      {
         ccd_name = theCCD[idx];
         ccd = indigo_devices [ccd_name];
         if (ccd != null && ccd.CCD_REMOVE_FITS_HEADER != null)
         {
            // indigo_log(log_prefix + "Removing weather data in FITS of " + ccd_name);
            ccd.CCD_REMOVE_FITS_HEADER.change ({KEYWORD: "AMBTEMP"});
            ccd.CCD_REMOVE_FITS_HEADER.change ({KEYWORD: "HUMIDITY"});
         }
      }
   }
}

indigo_event_handlers.Weather_handler = {
   devices: null,
   on_update: function(property)
   {
      var dev_name = property.device;
      var dev = indigo_devices[dev_name];
      if (property.name == 'CONNECTION' && property.state == "Ok")
      {
         if (property.items.DISCONNECTED)
         {
            ccdIdx = theCCD.indexOf(dev_name);
            if (ccdIdx >= 0)
            {
               indigo_log(log_prefix + "CCD " + dev_name + " removed, disconnected");
               theCCD.splice(ccdIdx, 1);
            }
            else if (theStation == dev_name)
            {
               indigo_log(log_prefix + "Weather station " + dev_name + " removed, disconnected");
               theStation = null;
               setWeatherInFITS(null);
            }
         }
         else if (property.items.CONNECTED)
         {
            var itf = dev.INFO.items.DEVICE_INTERFACE;
            if (itf != undefined)
            {
               if ((itf & (CCD_Mask | Agent_Mask)) == CCD_Mask
                   && !theCCD.indexOf(dev_name) >= 0)
               {
                  indigo_log(log_prefix + "CCD " + dev_name + " added");
                  theCCD.push(dev_name);
                  loadConfig(dev_name);
                  if (theStation != null)
                  {
                     weatherProp = indigo_devices[theStation].AUX_WEATHER;
                     if (weatherProp != null && weatherProp.state == "Ok")
                        setWeatherInFITS(weatherProp);
                  }
               }
               else if ((itf & (Weather_Mask | Agent_Mask)) == Weather_Mask
                        && theStation == null)
               {
                  indigo_log(log_prefix + "Weather station set to " + dev_name);
                  theStation = dev_name;
                  loadConfig(theStation);
                  weatherProp = indigo_devices[theStation].AUX_WEATHER;
                  if (weatherProp != null && weatherProp.state == "Ok")
                     setWeatherInFITS(weatherProp);
               }
            }
         }
      }
      else if (property.device == theStation && property.name=='AUX_WEATHER' && property.state == "Ok")
      {
         setWeatherInFITS(property);
      }
   },
   on_delete: function(property)
   {
      if (property.name == 'CONNECTION')
      {
         var dev_name = property.device;
         // indigo_log(property.name + " delete on " + dev_name);
         ccdIdx = theCCD.indexOf(dev_name);
         if (ccdIdx >= 0)
         {
            indigo_log(log_prefix + "CCD " + dev_name + " removed, deleted");
            theCCD.splice(ccdIdx, 1);
         }
         if (theStation == dev_name)
         {
            indigo_log(log_prefix + "Weather station " + dev_name + "removed, deleted");
            theStation = null;
            setWeatherInFITS(null);
         }
      }
   }
};
