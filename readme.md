OSGC
----

#### Open Source Game Collection

##### What is osgc?
OSGC is a frontend built with the [xygine](https://github.com/fallahn/xygine) library which acts as an interface to load any game written in the OSGC plugin format. I started this project as I'm often creating experiments and small games with xygine (Disclaimer - I'm the author ;) ), and creating a new project each time, even with a template available, becomes tedious. The plugin format allows creating a new xygine game quickly starting from only one header and one source file, while linking to xygine and SFML.

All games are open source, and come with the caveat that, well, most of them probably aren't very good. In fact they'll most likely be incomplete experiments. There may be a few gems, however, and I'm hoping that the open source nature will eventually encourage others to contribute to the games they like, or even write their own game plugins.


##### Getting Started


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

Plugin directories should have a unique name describing the game as this will be used by the front end as the display title if the info.xgi file is missing. The info.xgi file is a text file written in the xygine ConfigFile format. Its properties are used to display information on the browser screen of the front end.

    game_info
    {
        title = "My Game Title"
        thumb = "path/to/thumb.png"
        version = "0.0.1"
        author = "Jimmy Jim Bob"
    }

Note that these are all string properties and can in theory contain anything they like, with the exception of `thumb` which should be a relative path to a thumbanil image 960x540px in size. All properties are optional.


License
-------

The frontend project and OSGC plugin template source files are licensed under the zlib license. Further OSGC plugin source, such as the top down shooter game, may be licensed differently. See individual project directories for more information.