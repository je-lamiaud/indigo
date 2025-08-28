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

// Note : the SQM and Weather station may be the same AUX device, or not
var theCCD = [];
var theStation = null;
var theSQM = null;

const log_prefix = "FITS Updater: ";

const SQM_Kind     = 0x88000;
const CCD_Kind     = 0x000002;
const Weather_Kind = 0x408000;
const Agent_Kind   = 0x004000;
const SQM_Mask     = SQM_Kind | Agent_Kind;
const CCD_Mask     = CCD_Kind | Agent_Kind;
const Weather_Mask = Weather_Kind | Agent_Kind;

const temperatureKeyword = "AMBTEMP";
const humidityKeyword    = "HUMIDITY";
const pressureKeyword    = "PRESSURE";
const windSpeedKeyword   = "WINDSPD";
const windDirKeyword     = "WINDDIR";
const mpsasKeyword       = "MPSAS";

indigo_log(log_prefix + " starting");

function loadConfig(device)
{
   indigo_devices[device].CONFIG.change({LOAD: true});
}

function setMpsas(property)
{
   var mpsas = null;

   if (property != null)
      mpsas = property.items.SKY_BRIGHTNESS;

   for (i in theCCD)
   {
      var ccd = indigo_devices[theCCD[i]];
      if (ccd != null)
      {
         if (mpsas != null)
         {
            if (ccd.CCD_SET_FITS_HEADER != null)
            {
               // indigo_log(log_prefix + "Setting MPSAS to " + v + " in " + theCCD[i]);
               ccd.CCD_SET_FITS_HEADER.change({KEYWORD:mpsasKeyword, VALUE:mpsas.toFixed(2)});
            }
         }
         else
         {
            if (ccd.CCD_REMOVE_FITS_HEADER != null)
            {
               // indigo_log(log_prefix + "Removing MPSAS in " + theCCD[i]);
               ccd.CCD_REMOVE_FITS_HEADER.change({KEYWORD:mpsasKeyword});
            }
         }
      }
   }
}

function setWeather(property)
{
   var temperature = null;
   var humidity = null;
   var pressure = null;
   var windSpeed = null;
   var windDirection

   if (property != null)
   {
      temperature = property.items.TEMPERATURE;
      humidity = property.items.HUMIDITY;
      pressure = property.items.ATMOSPHERIC_PRESSURE;
      windSpeed = property.items.WIND_SPEED;
      windDirection = property.items.WIND_DIRECTION;
   }

   for (i in theCCD)
   {
      var ccd = indigo_devices[theCCD[i]];
      if (ccd != null)
      {
         if (ccd.CCD_SET_FITS_HEADER != null)
         {
            if (temperature != null)
               ccd.CCD_SET_FITS_HEADER.change ({KEYWORD: temperatureKeyword, VALUE: temperature.toFixed(1)});

            if (humidity != null)
               ccd.CCD_SET_FITS_HEADER.change ({KEYWORD: humidityKeyword, VALUE: humidity.toFixed(0)});

            if (pressure != null)
               ccd.CCD_SET_FITS_HEADER.change ({KEYWORD: pressureKeyword, VALUE: pressure.toFixed(0)});

            if (windSpeed != null)
            {
               windSpeed = windSpeed * 3.6; // convert m/s to km/h
               ccd.CCD_SET_FITS_HEADER.change ({KEYWORD: windSpeedKeyword, VALUE: windSpeed.toFixed(1)});
            }

            if (windDirection != null)
               ccd.CCD_SET_FITS_HEADER.change ({KEYWORD: windDirKeyword, VALUE: windDirection.toFixed(0)});
         }
         // Remove the FITS keywords for the null values if a keyword was previously set
         if (ccd.CCD_REMOVE_FITS_HEADER != null)
         {
            if (temperature == null && ccd.CCD_FITS_HEADERS.items[temperatureKeyword] != null)
               ccd.CCD_REMOVE_FITS_HEADER.change ({KEYWORD: temperatureKeyword});
            if (humidity == null && ccd.CCD_FITS_HEADERS.items[humidityKeyword] != null)
               ccd.CCD_REMOVE_FITS_HEADER.change ({KEYWORD: humidityKeyword});
            if (pressure == null && ccd.CCD_FITS_HEADERS.items[pressureKeyword] != null)
               ccd.CCD_REMOVE_FITS_HEADER.change ({KEYWORD: pressureKeyword});
            if (windSpeed == null && ccd.CCD_FITS_HEADERS.items[windSpeedKeyword] != null)
               ccd.CCD_REMOVE_FITS_HEADER.change ({KEYWORD: windSpeedKeyword});
            if (windDirection == null && ccd.CCD_FITS_HEADERS.items[windDirKeyword] != null)
               ccd.CCD_REMOVE_FITS_HEADER.change ({KEYWORD: windDirKeyword});
         }
      }
   }
}

indigo_event_handlers.FITS_Updater =
{
   devices: null,
	on_update: function(property)
	{
      var dev_name = property.device;
      var dev = indigo_devices[dev_name];
      if (property.name == "CONNECTION" && property.state == "Ok")
      {
         if (property.items.DISCONNECTED)
         {
            ccdIdx = theCCD.indexOf(dev_name);
            if (ccdIdx >= 0)
            {
               indigo_log(log_prefix + "CCD " + dev_name + " removed, disconnected");
               theCCD.splice(ccdIdx, 1);
            }
            if (theStation == dev_name)
            {
               indigo_log(log_prefix + "Weather station " + dev_name + " removed, disconnected");
               theStation = null;
               setWeather(null);
            }
            if (theSQM == dev_name)
            {
               indigo_log(log_prefix + "SQM " + property.device + " removed, disconnected");
               theSQM = null;
               setMpsas(null);
            }
         }
         else if (property.items.CONNECTED)
         {
            var itf = dev.INFO.items.DEVICE_INTERFACE;
            if (itf != undefined)
            {
               if ((itf & CCD_Mask) == CCD_Kind && !theCCD.indexOf(dev_name) >= 0)
               {
                  indigo_log(log_prefix + "CCD " + dev_name + " added");
                  theCCD.push(dev_name);
                  loadConfig(dev_name);
                  if (theStation != null)
                  {
                     var weatherProp = indigo_devices[theStation].AUX_WEATHER;
                     if (weatherProp != null && weatherProp.state == "Ok")
                        setWeather(weatherProp);
                  }
                  if (theSQM != null)
                  {
                     var sqmProp = indigo_devices[theSQM].AUX_WEATHER;
                     if (sqmProp != null && sqmProp.state == "Ok")
                        setMpsas(sqmProp);
                  }
               }
               if ((itf & Weather_Mask) == Weather_Kind && theStation == null)
               {
                  indigo_log(log_prefix + "Weather station set to " + dev_name);
                  theStation = dev_name;
                  loadConfig(theStation);
                  var weatherProp = indigo_devices[theStation].AUX_WEATHER;
                  if (weatherProp != null && weatherProp.state == "Ok")
                     setWeather(weatherProp);
               }
               if ((itf & SQM_Mask) == SQM_Kind && theSQM == null)
               {
                  indigo_log(log_prefix + "SQM set to " + dev_name);
                  theSQM = dev_name;
                  loadConfig(dev_name);
                  var sqmProp = indigo_devices[theSQM].AUX_WEATHER;
                  if (sqmProp != null && sqmProp.state == "Ok")
                     setMpsas(sqmProp);
               }
            }
         }
      }
      else if (property.name == "AUX_WEATHER" && property.state == "Ok")
      {
         if (dev_name == theStation)
            setWeather(property);
         if (dev_name == theSQM)
            setMpsas(property);
      }
   },
   on_delete: function(property)
   {
      if (property.name == "CONNECTION")
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
            indigo_log(log_prefix + "Weather station " + dev_name + " removed, deleted");
            theStation = null;
            setWeather(null);
         }
         if (theSQM == dev_name)
         {
            indigo_log(log_prefix + "SQM " + dev_name + " removed, deleted");
            theSQM = null;
            setMpsas(null);
         }
      }
   }
};
