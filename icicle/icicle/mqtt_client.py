"""MQTTClient class for publishing measurements to MQTT broker."""

import logging

from paho.mqtt.client import Client, CallbackAPIVersion

logger = logging.getLogger(__name__)


class MQTTClient:
    """Implementation of Eclipse Paho MQTT Python client."""

    def __init__(self, clientID="", ssl=True):
        """Initialises a MQTT client instance and sets default connect and disconnect
        callbacks.

        :param clientID: Unique client ID string used when connecting to the broker.
            If `clientID` is zero length or None, then one will be randomly generated.
        """

        def on_connect(client, userdata, flags, reason_code, properties):
            if reason_code == 0:
                logger.info("Connected to MQTT broker!")
            else:
                logger.error(f"Failed to connect, reason code {reason_code}\n")

        def on_disconnect(client, userdata, flags, reason_code, properties):
            if reason_code == 0:
                logger.info("Disconnected from MQTT broker!")
            else:
                logger.error(
                    "Unexpected disconnection from MQTT broker, "
                    f"reason code {reason_code}\n"
                )

        self.clientID = clientID
        self.client = Client(CallbackAPIVersion.VERSION2, clientID)
        if ssl:
            self.client.tls_set_context(context=None)
            self.client.tls_insecure_set(True)
        self.client.on_connect = on_connect
        self.client.on_disconnect = on_disconnect

    def __del__(self):
        """Deconstructor."""
        self.disconnect()

    def connect(self, broker, port=1883, username=None, password=None):
        """Connects the client to a broker.

        :param broker: Hostname or IP address of the remote broker.
        :param port: Network port of the server host to connect to. Defaults to 1883.
        :param username: Sets username for broker authentification. Defaults to `None`.
        :param password: Sets password for broker authentification. Defaults to `None`.
        """
        if not (username is None or password is None):
            self.client.username_pw_set(username, password)
        self.client.connect(broker, port)

    def disconnect(self):
        self.client.disconnect()

    def publish(self, topic, msg):
        """Sends a message from the client to the broker.

        :param topic: A string specifiying the topic the message should be published on.
        :param msg: A string specifying the  message to send. Passing an `int` or
            `float` will result in the message being converted to a string.
        """
        result = self.client.publish(topic, msg)
        logger.debug(f"Published {topic}: {msg}")
        logger.debug(f"Result: {result}")
        status = result[0]
        if status != 0:
            print(f"Failed to send message to topic {topic}")

    def subscribe(self, sub, callback=None):
        """Subscribes the client to a topic and sets a default message callback.

        :param sub: A string specifying the topic to subscribe to.
        :param callback: F-string specifying the `on_message` callback for the
            subscribed topic. If `None` the global `on_message` callback will be set to
            `f"Received '{msg.payload.decode()}' from '{msg.topic}'"`.
            Defaults to `None`.
        """
        if callback is None:

            def on_message(client, userdata, msg):
                print(f"Received '{msg.payload.decode()}' from '{msg.topic}'")

            self.client.on_message = on_message
        else:

            def on_message_sub(client, userdata, msg):
                print(callback)

            self.client.message_callback_add(sub, on_message_sub)

        self.client.subscribe(sub)

    def loop_start(self):
        """Start client loop.

        Handles reconnecting automatically.
        """
        self.client.loop_start()

    def loop_stop(self):
        """Stop client loop."""
        self.client.loop_stop()

    def loop_forever(self):
        """Loop client forever.

        Handles reconnecting automatically. Loop will not return until client calls
        `disconnect()`.
        """
        self.client.loop_forever()
