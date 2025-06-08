## Proyecto : Integración de Comunicación LoRa para el Control Autónomo del Drone DJI Tello usando Heltec V3

Este repositorio contiene la información sobre cómo controlar movimientos básicos de un Dron Tello usando módulos Heltec V3. El esquema implementado es el siguiente: 

<div align="center">
  <img src="resources/images/esquema.png" alt="Esquema del Proyecto" width="65%"/>
</div>

Este proyecto está realizado en Arduino IDE y consiste en controlar un dron DJI Tello usando LoRa, con el objetivo de enviar instrucciones desde un control hacia el dron a larga distancia. El esquema del funcionamiento del sistema se presenta en la Figura anterior. Para la ejecución de cada instrucción en el dron es necesario primero que el emisor reciba mediante un teclado qué comandos se desea enviar. El envío de estos comandos hacia el receptor que tiene conexión con el dron se realiza mediante mensajes LoRa en forma de cadena de texto, para lo cual se ha realizado un diccionario, que es usado por el receptor para definir qué instrucción será enviada al dron según el mensaje que ha recibido.

Para la implementación de LoRa se usa la librería SX126x-Arduino disponible en [este repositorio de GitHub](https://github.com/ElectronicCats/LoRaWAN-SX126x). El repositorio detalla las configuraciones de LoRa necesarias para los parámetros como frecuencia de operación, potencia de transmisión, factor de ensanchamiento, etc. Para la conexión del módulo receptor con el Dron Tello se usa la librería telloArduino disponible en [este repositorio en GitHub](https://github.com/akshayvernekar/telloArduino).

### Sistema de control

El sistema de control del Dron DJI Tello ofrece dos opciones de vuelo disponibles por el momento (la opción de vuelo en tiempo real aún no está implementada), la primera corresponde a una ejecución de un plan de vuelo preprogramado almacenado en la memoria del microcontrolador, mientras que la segunda funciona al establecer un vuelo autónomo mediante interacción con el Heltec LoRa, permitiendo la creación de una ruta dinámica.

<div align="center">
  <img src="resources/images/control.png" alt="Diagrama de flujo para el control de los escenarios de vuelo" width="65%"/>
</div>

La figura anterior muestra las dos opciones de vuelo programadas para el control del dron y presenta una tercera opción correspondiente al control en tiempo real. Esta opción se incluye porque el sistema de control está diseñado con la proyección de incluir este modo de operación. Sin embargo, al momento de entregar este informe de proyecto, la programación de esta función no está finalizada. Por ello, no se incluye como una opción dentro de los planes de vuelo del dron, sino como una proyección para futuros desarrollos del proyecto.

### Circuito del control 
Para el sistema de control se ha implementado un control remoto que permite enviar la instrucciones al Dron Tello. Las conexiones del circuito para el control remoto se presentan a continuación:
<div align="center">
  <img src="resources/images/circuito.jpg" alt="Conexiones circuito" width="70%"/>
</div>


### Sistema de recepción

El sistema de recepción está compuesto por:

- Un módulo Heltec V3 
- Una antena para la comunicación mediante LoRa
- Una batería para alimentación del Heltec V3
- El dron DJI Tello
- Un soporte impreso en 3D para la sujeción del módulo receptor LoRa en el dron DJI Tello.

El receptor utiliza comunicación LoRa y Wifi, las cuales permiten recibir los mensajes de control desde el emisor y enviar las instrucciones al dron Tello respectivamente. En la siguiente tabla se presentan los comandos enviados al dron Tello según el mensaje recibido mediante LoRa.

En la siguiente tabla se presentan los comandos implementados y probados, dado el mensaje enviado por el Heltec V3 mediante LoRa, el HeltecV3 receptor envía mediante Wifi las instrucciones al dron Tello. En este proyecto se implementaron las instrucciones básicas, sin embargo, es posible agregar nuevas funcionalidades al sistema considerando los movimientos que se encuentran en el Manual del DJI Tello

| **Mensaje Recibido (LoRa)** | **Comando Ejecutado (Tello)** | **Descripción** |
|-----------------------------|-------------------------------|-----------------|
| UP,50                       | tello.up(distancia);           | Hace que el dron suba 50 cm. |
| DOWN,30                     | tello.down(distancia);         | Hace que el dron baje 30 cm. |
| LEFT,20                     | tello.left(distancia);         | Hace que el dron se mueva hacia la izquierda 20 cm. |
| RIGHT,10                    | tello.right(distancia);        | Hace que el dron se mueva hacia la derecha 10 cm. |
| FORWARD,100                 | tello.forward(distancia);      | Hace que el dron avance hacia adelante 100 cm. |
| BACK,80                     | tello.back(distancia);         | Hace que el dron retroceda 80 cm. |
| TAKEOFF                     | tello.takeoff();               | Hace que el dron despegue. |
| LAND                        | tello.land();                  | Hace que el dron aterrice. |

En la siguiente figura se presenta el funcionamiento del programa.

<div align="center">
  <img src="resources/images/receptor.png" alt="Diagrama de flujo del proceso en el receptor" width="45%"/>
</div>

## Diseño de elemento de sujeción 
Para el montaje del módulo Heltec V3 sobre el dron Tello, se ha diseñado un soporte que permite la sujeción de la antena de comunicación LoRa y una batería de alimentación para el módulo LoRa. El diseño en 3D, realizado en Autodesk Fusion 360, asegura que cada componente (Heltec V3, batería de 3.7 V y antena LoRa) esté centrado en el elemento de sujeción. Esto mantiene el centro de masa alineado con el centro del soporte, distribuyendo uniformemente el peso entre los motores del dron DJI Tello. 

Este repositorio también incluye los diseños de estos elementos de sujeción. 
<div align="center">
  <img src="resources/images/sujecion.jpg" alt="Diseño" width="45%"/>
</div>

## Trama de envío entre Heltec emisor y receptor 
Para el envío de datos se definió un formato de trama. La trama es un vector de elementos uint8_t, el tamaño de esta trama depende de la cantidad de instrucciones a enviar. El formato de trama es el siguiente:

1. **Primer byte o byte 0 de la trama**: Indica la cantidad total de instrucciones, sin contar las instrucciones "TAKEOFF" y "LAND" para despegue y aterrizaje. Si solo se desea realizar el despegue y aterrizaje, este primer byte deberá tener todos sus bits en 1 (`0xFF`), lo cual indicará que se desea ejecutar únicamente las instrucciones "TAKEOFF" y "LAND". Hasta el momento se ha definido 4 bits para la cantidad de instrucciones, es decir 16 instrucciones, por lo tanto se pueden enviar hasta 16 instrucciones para una ruta, sin embargo, esto puede ser desarrollado posteriormente. 

2. **Instrucciones con parámetros**: Las instrucciones a enviar utilzian dos bytes para definir la instrucción y el parámetro de distancia en centímetros a recorrer.
    - **Dos bytes** son utilizados por cada instrucción. 
        - El **primer byte** contiene los 4 bits más significativos para la instrucción. Los 4 bits utilizados para la instrucción permiten gestionar hasta 16 instrucciones diferentes. No obstante, en este proyecto, solo se utilizan las 6 instrucciones básicas:
          - **RIGHT**
          - **LEFT**
          - **UP**
          - **DOWN**
          - **FORWARD**
          - **BACK**
        - El bit menos significativo del primer byte junto con los 8 bits del segundo byte se utilizan para el parámetro de la instrucción, es decir, la distancia en centímetros. Esto da un total de 9 bits asignados para la distancia, lo que permite un valor máximo de 512. Sin embargo, el rango máximo permitido por el dron es de 500 centímetros.

<div align="center">
  <img src="resources/images/trama.png" alt="Trama de envío mediante LoRA" width="80%"/>
</div>


## Compatibilidad de códigos
Para que el código del emisor pueda ser cargado en el Heltec V3, es necesario agregar la Board del Heltec a Arduino IDE, posteriormente se debe seleccionar la placa Wifi LoRa 32(V3) como se muestra en la siguiente figura.

<div align="center">
  <img src="resources/images/placa_heltec.png" alt="Placa emisor" width="65%"/>
</div>

## Recursos útiles

- [Conexión del ESP32 con el dron Tello (Repositorio de ejemplo)](https://github.com/sagar-koirala/TelloESP32/tree/master)
