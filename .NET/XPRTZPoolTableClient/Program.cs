using System.Net.WebSockets;
using Websocket.Client;

var poolTableIp = "192.168.1.225";
var poolTablePort = 81;

var url = new Uri($"ws://{poolTableIp}:{poolTablePort}");

var client = new WebsocketClient(url);
client.ReconnectTimeout = TimeSpan.FromSeconds(30);
client.ReconnectionHappened.Subscribe(info =>
    Console.WriteLine($"Reconnection happened, type: {info.Type}"));

client.MessageReceived.Subscribe(msg =>
{
    if (msg.Text == null)
        return;

    var message = msg.Text;

    if (!message.Contains(','))
        return;

    if (message.StartsWith("hit"))
    {
        Console.WriteLine("Received a hit!");
        var data = message.Split(",");
        var player = Convert.ToInt32(data[1]);
        var x = Convert.ToInt32(data[2]);
        var y = Convert.ToInt32(data[3]);
        var z = Convert.ToInt32(data[4]);
        
        Console.WriteLine($"Hit received for player: {player}. Impact force: x: {x}, y: {y}, z: {z}");
        return;
    }

    if (message.StartsWith("pocket"))
    {
        var hitData = message.Split(",");
        var pocket = Convert.ToInt32(hitData[1]);
        
        Console.WriteLine($"Received ball in pocket {pocket}");
    }
});

client.Start();

while (true)
{
    Console.Write("Send message to server: ");
    var k = Console.ReadLine();

    if (k == null)
        continue;
    
    if (k == "exit")
    {
        client.Stop(WebSocketCloseStatus.NormalClosure, "Stop");
        break;
    }

    client.Send(k);
}