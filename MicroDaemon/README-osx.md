
Extras for Info.plist
=====================

For hiding the UI of the application, add in Info.plist the:

    <key>LSUIElement</key>
    <string>1</string>

For handling the `cernvm-webapi` URL scheme, add in Info.plist the:

	<key>CFBundleURLTypes</key>
	<array>
	        <dict>
	                <key>CFBundleTypeRole</key>
	                <string>Viewer</string>
	                <key>CFBundleURLName</key>
	                <string>CernVM WebAPI</string>
	                <key>CFBundleURLSchemes</key>
	                <array>
	                        <string>cernvm-webapi</string>
	                </array>
	        </dict>
	</array>

