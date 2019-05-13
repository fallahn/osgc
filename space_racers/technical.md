A few notes on things I'm bound to forget:

##### Network game initialisation
Assumes clients are in lobby state with one acting as a host, running a server instance also in a lobby state.

Host sends game data to server:

    PacketID::LobbyData;
    struct LobbyData final
    {
        std::uint64_t peerIDs[4]; //list of connected peers to map to vehicle type
        std::uint8_t vehicleIDs[4]; //type of vehicle for each player
        std::uint8_t playerCount = 1;
        std::uint8_t mapIndex = 0;
        std::uint8_t lapCount = 0;
        std::uint8_t gameMode = 0; //dictates which state the server should switch to
    }; 

on receipt the server stores this information in the shared data struct (shared between server states) and then requests a new game state based on the game mode

The new server state loads the map information and creates server side player entities. When this is complete it sends

    PacketID::GameStart;
    struct GameStart final
    {
        std::uint8_t playerCount = 1; //so the client knows it has received all player info
        std::uint8_t mapIndex = 0;
        std::uint8_t gameMode = 0;
    };

on success, else and error packet ID if it fails. If the result is successful the client side lobby state stores the above information in the client shared data struct and requests a new game state based on the game mode.

The client game state loads the resources and the map based on the current set of shared data. When map loading is complete it sends

    PscketID::ClientMapLoaded;

TODO this needs to quit on failure, probably also notifying the server. I haven't decided how the server should handle player drop-outs yet.

The server game state then iterates over the expected player list, and their vehicle types and spawn positions. For each player the following is sent

    PacketID::VehicleData;
    struct VehicleData final
    {
        std::uint64_t peerID = 0;
        std::uint32_t serverEntityID = 0; //for clients side tracking when receiving updates
        float x = 0.f; float y = 0.f; //initial position
        std::uint8_t vehicleType = 0;
    };

When the client receives each one it is counted, and checked to see if the peerID matches the client. If the peerID matches then the `InputParser` for player control is also added. Vehicle entities are mapped to `serverEntityID` so that incoming updates from the server are easily applied. Once the clients vehicle count matches the player count the client sends a `Ready` packet.

When the server sees all clients are ready a 'count -in' packet is broadcast to all clients to trigger the race start.
