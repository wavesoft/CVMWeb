
Extras for Info.plist
=====================

You should add the extra parameters:

    <key>LSUIElement</key>
    <string>1</string>
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

