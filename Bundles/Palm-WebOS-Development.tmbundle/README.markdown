Palm webOS TextMate bundle
==========================


Installation
------------

To install via Git:

    mkdir -p ~/Library/Application\ Support/TextMate/Bundles
    cd ~/Library/Application\ Support/TextMate/Bundles
    git clone git://github.com/devilx/palm-webos-development-tmbundle.git "Palm webOS.tmbundle"
    osascript -e 'tell app "TextMate" to reload bundles'

To install without Git:

    mkdir -p ~/Library/Application\ Support/TextMate/Bundles
    cd ~/Library/Application\ Support/TextMate/Bundles
    wget http://github.com/devilx/palm-webos-development-tmbundle/tarball/master
    tar zxf devilx-palm-webos-development-tmbundle*.tar.gz
    rm devilx-palm-webos-development-tmbundle*.tar.gz
    mv devilx-palm-webos-development-tmbundle* "Palm webOS.tmbundle"
    osascript -e 'tell app "TextMate" to reload bundles'
		
Source can be viewed or forked via GitHub: [http://github.com/devilx/palm-webos-development-tmbundle/tree/master](http://github.com/devilx/palm-webos-development-tmbundle/tree/master)