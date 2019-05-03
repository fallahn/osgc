OSGC
----

#### Open Source Game Collection

##### What is osgc?
OSGC is a frontend built with the [xygine](https://github.com/fallahn/xygine) library which acts as an interface to load any game written in the OSGC plugin format. I started this project as I'm often creating experiments and small games with xygine (Disclaimer - I'm the author ;)), and creating a new project each time, even with a template available, can become tedious. The plugin format allows creating a new xygine game quickly starting from only one header and one source file, while linking to xygine and SFML.

All games are open source, and come with the caveat that, well, most of them probably aren't very good. In fact they'll most likely be incomplete experiments. There may be a few gems, however, and I'm hoping that the open source nature will eventually encourage others to contribute to the games they like, or even write their own game plugins.


##### Getting Started
Dependencies: You'll need to have at least SFML 2.5.x built and installed as well as [xygine](https://github.com/fallahn/xygine) to build the frontend. Different plugins may require further libraries, for example the top down shooter plugin also requires [tmxlite](https://github.com/fallahn/tmxlite). Use the `CMakeLists.txt` in the root of the repository to create project files for your build chain of choice - OSGC is frequently tested on Windows, linux and macOS with Visual Studio, gcc and Apple clang. The output of the binary files may not necessarily meet the directory requirement of OSGC (see below) but it is usually quickly fixed with some symlinks (mklink works just as well on Windows).

To create your own plugin copy the plugin_template directory and rename it to something of your choice. Edit the included `CMakeLists.txt` for your new project, or create a new project in your IDE if you prefer. Remember to add the template source files to your project, link against SFML and xygine (and any other libraries you require) and set the project to be built as a shared library. The library output file must be named osgc.dll/libosgc.so/libosgc.dylib depending on your platform.

Use the `begin()` and `end()` functions in the `EntryPoint.cpp` file to register and unregister any game states with the frontend. See the plugin example in the 'shooter' directory for further example. To learn more about creating a game with xygine follow the [xygine tutorial](https://github.com/fallahn/xygine/tree/master/tutorial) and refer to the [wiki](https://github.com/fallahn/xygine/wiki).

Note that while OSGC is designed around xygine it is not strictly necessary to use xygine beyond implementing the plugin interface. Any game or software which will run within the evt/update/draw loop of a xygine state and can be drawn to an SFML render window is entirely feasible. For example my Gameboy emulator Speljongen [can be built as an OSGC plugin](https://github.com/fallahn/speljongen/tree/master/spel-osgc).

##### Directory Structure
To correctly load plugins and their assets, the directory structure must follow this layout:

    osgc
      |
      |____osgc-frontend
      |____assets
      |       |____assets used by the front end
      |
      |____plugins
              |____directory named for plugin
              |               |________________osgc.dll/so/dylib
              |               |________________info.xgi
              |               |________________assets
              |                                   |______assets used by plugin
              |
              |____directory named for plugin
                              |________________as above

Plugin directories should have a unique name describing the game as this will be used by the front end as the display title if the info.xgi file is missing. The info.xgi file is a text file written in the xygine `ConfigFile` format. Its properties are used to display information on the browser screen of the front end.

    game_info
    {
        title = "My Game Title"
        thumb = "path/to/thumb.png"
        background = "path/to/background.png"
        version = "0.0.1"
        author = "Jimmy Jim Bob"
    }

Note that these are all string properties and can in theory contain anything they like, with the exception of `thumb` which should be a relative path to a thumbnail image 960x540px in size, and `background` which should be a relative path to an image 1920x1080 in size. All properties are optional.

See the [screens](/screens) directory for screenshots of OSGC in action

License
-------

The frontend project and OSGC plugin template source files are licensed under the zlib license. Further OSGC plugin source, such as the top down shooter game, may be licensed differently. See individual project directories for more information.