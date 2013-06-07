#/**********************************************************\ 
#
# Auto-Generated Plugin Configuration file
# for CernVM Web API
#
#\**********************************************************/

set(PLUGIN_NAME "CVMWeb")
set(PLUGIN_PREFIX "CVWA")
set(COMPANY_NAME "CernVM")

# ActiveX constants:
set(FBTYPELIB_NAME CVMWebLib)
set(FBTYPELIB_DESC "CVMWeb 1.0 Type Library")
set(IFBControl_DESC "CVMWeb Control Interface")
set(FBControl_DESC "CVMWeb Control Class")
set(IFBComJavascriptObject_DESC "CVMWeb IComJavascriptObject Interface")
set(FBComJavascriptObject_DESC "CVMWeb ComJavascriptObject Class")
set(IFBComEventSource_DESC "CVMWeb IFBComEventSource Interface")
set(AXVERSION_NUM "1")

# NOTE: THESE GUIDS *MUST* BE UNIQUE TO YOUR PLUGIN/ACTIVEX CONTROL!  YES, ALL OF THEM!
set(FBTYPELIB_GUID 1d407c5e-b65c-5a89-8f1f-a269c58d2e49)
set(IFBControl_GUID bad19058-e74b-5669-bcdf-3ecbd6a94d8e)
set(FBControl_GUID 3300d921-2cce-5903-85aa-947fda74fb46)
set(IFBComJavascriptObject_GUID 0d08e943-83ad-597c-af5c-b665c436991c)
set(FBComJavascriptObject_GUID 212a544e-a5de-519a-8a4c-f533800cdee2)
set(IFBComEventSource_GUID 4b7e1348-cc10-5b71-8a59-6b19de2318d3)
if ( FB_PLATFORM_ARCH_32 )
    set(FBControl_WixUpgradeCode_GUID c5dc6f66-3342-5539-8958-92a9212085fc)
else ( FB_PLATFORM_ARCH_32 )
    set(FBControl_WixUpgradeCode_GUID ff7309fd-a4a4-5a5f-abe6-1c5bb76018f9)
endif ( FB_PLATFORM_ARCH_32 )

# these are the pieces that are relevant to using it from Javascript
set(ACTIVEX_PROGID "CernVM.CVMWeb")
set(MOZILLA_PLUGINID "cernvm.cern.ch/CVMWeb")

# strings
set(FBSTRING_CompanyName "CernVM Group - CERN")
set(FBSTRING_PluginDescription "CernVM Web Access Plugin")
set(FBSTRING_PLUGIN_VERSION "1.1.5")
set(FBSTRING_LegalCopyright "Copyright 2013 CernVM Group - CERN")
set(FBSTRING_PluginFileName "np${PLUGIN_NAME}.dll")
set(FBSTRING_ProductName "CernVM Web API")
set(FBSTRING_FileExtents "")
if ( FB_PLATFORM_ARCH_32 )
    set(FBSTRING_PluginName "CernVM Web API")  # No 32bit postfix to maintain backward compatability.
else ( FB_PLATFORM_ARCH_32 )
    set(FBSTRING_PluginName "CernVM Web API_${FB_PLATFORM_ARCH_NAME}")
endif ( FB_PLATFORM_ARCH_32 )
set(FBSTRING_MIMEType "application/x-cvmweb")

# Uncomment this next line if you're not planning on your plugin doing
# any drawing:

set (FB_GUI_DISABLED 1)

# Mac plugin settings. If your plugin does not draw, set these all to 0
set(FBMAC_USE_QUICKDRAW 0)
set(FBMAC_USE_CARBON 0)
set(FBMAC_USE_COCOA 0)
set(FBMAC_USE_COREGRAPHICS 0)
set(FBMAC_USE_COREANIMATION 0)
set(FBMAC_USE_INVALIDATINGCOREANIMATION 0)

# If you want to register per-machine on Windows, uncomment this line
#set (FB_ATLREG_MACHINEWIDE 1)

# Curl library for content fetching
add_firebreath_library(curl)
add_firebreath_library(openssl)
add_firebreath_library(jsoncpp)

