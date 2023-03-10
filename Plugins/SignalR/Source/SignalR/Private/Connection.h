/*
 * MIT License
 *
 * Copyright (c) 2020-2022 Frozen Storm Interactive, Yoann Potinet
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "CoreMinimal.h"
#include "ConnectionErrorData.h"
#include "IWebSocket.h"
#include "Interfaces/IHttpRequest.h"

class SIGNALR_API FConnection : public TSharedFromThis<FConnection>
{
public:
    FConnection(const FString& InHost);

    void Connect(bool InReconnecting);

    bool IsConnected();

    bool WasReconnectingOnNegotiate() const { return ReconnectingOnNegotiate; }

    void Send(const FString& Data);

    void Close(int32 Code = 1000, const FString& Reason = FString());

    IWebSocket::FWebSocketConnectedEvent& OnConnected();

    IWebSocket::FWebSocketConnectionErrorEvent& OnConnectionError();

    IWebSocket::FWebSocketClosedEvent& OnClosed();

    IWebSocket::FWebSocketMessageEvent& OnMessage();

    FOnHubNegotiationCompleteSuccessEvent* OnNegotiationCompleteSuccess() { return &OnNegotiationCompleteHandlerSuccess; };
    FOnHubNegotiationCompleteFailEvent* OnNegotiationCompleteFail() { return &OnNegotiationCompleteHandlerFail; };

    void AddNegotiateHeader(const FString& InKey, const FString& InValue);
    void AddWebSocketHeader(const FString& InKey, const FString& InValue);

    void ClearNegotiateHeader();
    void ClearWebSocketHeader();

    void StartWebSocket(const FString& InWebSocketHost);
    void StartWebSocket();

private:
    void Negotiate(bool InReconnecting);
    void OnNegotiateResponse(FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bConnectedSuccessfully);
    //void OnNegotiateResponseOld(FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bConnectedSuccessfully);

    static bool CheckAvailableTransports(TSharedPtr<FJsonObject> InJsonObject);

    TSharedPtr<IWebSocket> Connection;
    FString Host;

    TMap<FString, FString> NegotiateHeaders;
    TMap<FString, FString> WebSocketHeaders;

    IWebSocket::FWebSocketConnectedEvent OnConnectedEvent;
    IWebSocket::FWebSocketConnectionErrorEvent OnConnectionErrorEvent;
    IWebSocket::FWebSocketClosedEvent OnClosedEvent;
    IWebSocket::FWebSocketMessageEvent OnMessageEvent;

    FString ConnectionToken;
    FString ConnectionId;

    FOnHubNegotiationCompleteSuccessEvent OnNegotiationCompleteHandlerSuccess;
    FOnHubNegotiationCompleteFailEvent OnNegotiationCompleteHandlerFail;

    bool ReconnectingOnNegotiate = false;

    static FString ConvertToWebsocketUrl(const FString& Url);
};
