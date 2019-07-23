Mapping Guide
-------------

Maps are [Tiled](https://www.mapeditor.org) `.tmx` format maps, stored with base64 encoding and zlib compression. Object types are defined in objecttypes.xml.

Maps can have any size tiles, but tilesets should all have the same tile size and must be no more than 16x16 (256 total) tiles in size. Multiple tilesets can be used per map, but only one tile set is currently supported per layer. With this in mind it's best to organise your tilesets such as one for basic geometry, another for details and so on. Also, as Tiled starts tile IDs at 1, not 0, the last tile (256) will currently not be drawn. This may change in the future, however.

Objects have specific properties which define their behaviour. For example collectible objects have an ID property which defines their type.

  * 0 is a coin
  * 1 is a shield
  * 2 is ammo
  * 3 is extra life

Prop objects are not interactable and are placed in the background between the furthest map layer and the next layer

  * 0 is a waterfall
  * 1 is a lavafall
  * 2 is a torch/lamp
  * 3 is a crate spawner

Enemy objects also have an ID property. See the existing maps for further information (as these are likely to change with updates). The IDs ahould match the enum values in EnemySystem.hpp

Maps must have at least one checkpoint object, which is where the player will start the level, with an ID of 0. Further checkpoints should have their ID incremented each time. When the map is loaded the checkpoint sprite is automatically placed based on the current theme.

Maps must have exactly one exit. Exit properties are:

  * ID. 0 is a regular exit with no sprite. With this exit the level will transition directly to the next map. 1 is a 'stage end'. The stage end sprite is drawn here automatically, and when the player exits through this object a 'round summary' screen is first displayed before the next map is loaded. Usually used for switching to a new theme.
  * next_map the name of the next map to load, without the .tmx extension
  * theme is the theme to be used by the next map

Themes are stored in a directory matching the theme name within the assets/images/ directory and assets/sprites/ directory. Currently `gearboy` and `mes` themes exist. Adding a new theme is just a case of creating a new directory with a new theme name, alongside the gearboy and mes directories. The assets themselves must have the same names as the other themes, eg `checkpoint.spt` or `player.spt`. Themes are used to replace sprites for the player, enemies, collectibles and other details.

Dialogue objects trigger an on-screen dialogue box used to expose story lines. They have one property `file` which contains the name of the text file in the dialogue directory to display. These should be used sparingly to not distract from the gameplay too much. It can be jarring to have the flow of gameplay interrupted by an unexpected tex box.

All objects should be rectangular with the exception of enemies. Enemies use polyline objects which mark the start and end of their travel path. If a polyline with more than two points is drawn then the enemy will move in a straight line between the first and last point.

Most of these rules are early in development and are subject to change. I'd love to hear feedback and suggestions for improving the mapping process.