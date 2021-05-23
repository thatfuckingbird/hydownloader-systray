# hydownloader-systray

Remote management GUI for hydownloader.

## What is this and how it works?

This is a graphical applications to remotely manage a hydownloader instance.
After setting the hydownloader API address and access key, this can be used to manage URLs, manage subscriptions,
view logs or do basic maintenance actions.

## Build instructions

```
cd hydownloader-systray
mkdir build
cd build
cmake ..
make
```

## How to use

Customize the `settings.ini` found in this repo, then launch with `hydownloader-systray --settings /path/to/settings.ini`.

Command line flags:

* `--settings <filename>`: specify location of settings file
* `--startVisible`: start with the main window visible

## TODO

* Color the rows of the log view according to severity
* Add icon and desktop file
* Add log rotation

## License

hydownloader-systray is licensed under the Affero General Public License v3+ (AGPLv3+).
