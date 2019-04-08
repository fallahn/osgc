OSGC
----

#### Open Source Game Collection

###### What is osgc?
OSGC is a frontend built with the [xygine]() library which acts as an interface to load any game written in the OSGC plugin format. I started this project as I'm often creating experiments and small games with xygine (Disclaimer - I'm the author ;) ), and creating a new project each time, even with a template available, becomes tedious. The plugin format allows creating a new xygine game quickly starting from only one header and one source file, while linking to xygine and SFML.

All games are open source, and come with the caveat that, well, most of them probably aren't very good. In fact they'll most likely be incomplete experiments. There may be a few gems, however, and I'm hoping that the open source nature will eventually encourage others to contribute to the games they like, or even write their own game plugins.


###### Getting Started


###### Directory Structure
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
              |               |________________assets
              |                                   |______assets used by plugin
              |
              |____directory named for plugin
                              |________________as above

