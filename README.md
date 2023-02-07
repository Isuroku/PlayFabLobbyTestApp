# PlayFabLobbyTestApp
This is a UE4_27 version test example. MM and create findable lobby with realtime notification.

This is a standard Playfab lobby creation using their plugin.
## Real-time notifications for Lobby
Not obvious, but in order to make a lobby created by a player visible in the search for other players, you need to subscribe this lobby to tracking change. [PlayFab Doc](https://github.com/MicrosoftDocs/playfab-docs/blob/docs/playfab-docs/features/real-time-notifications/subscribing-to-resources.md)

```C#
    //Cutted example from code

    Request.Type = PlayFab::MultiplayerModels::SubscriptionTypeLobbyChange;
	Request.pfEntityKey.Id = PlayfabTitlePlayerID_;
	Request.pfEntityKey.Type = PlayfabTitlePlayerIDFieldName;
	Request.ResourceId = LobbyId_;

	Request.PubSubConnectionHandle = PubSubRequester_->GetConnectionHandleString();

	PlayFab::UPlayFabMultiplayerAPI().SubscribeToLobbyResource(Request, OnSuccess, OnError);
```

For support real-time notification PlayFab use web-socket based technology which has name [SignalR](https://dotnet.microsoft.com/en-us/apps/aspnet/signalr?). [One more link.](https://learn.microsoft.com/en-us/gaming/playfab/features/real-time-notifications/signalr-hub)

For support SignalR in this project I use plugin [Unreal-SignalR](https://github.com/FrozenStormInteractive/Unreal-SignalR) from Frozen Storm Interactive, Yoann Potinet.

I've changed this plugin. Here's why.
Playfab requests the app identify information on negotiate step. I devided authenficate info for negotiation and for next step (connect to web-socket).
After this Playfab makes redirect to other address for web-socket. In this moment we need to add the auth-token from previous step. I changed code of the plugin for these things too.

After this I invoke the command "StartOrRecoverSession" through SignalR and take "PubSubConnectionHandle" string for "SubscribeToLobbyResource".