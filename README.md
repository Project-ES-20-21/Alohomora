# Alohomora
Een 4-digit cijferslot.

## Uitvoering
Aan de hand van een ESP-WROOM-32 en een touchscreen LCD als basis wordt een 4-digit cijferslot ontworpen dat een solenoïde aanstuurt zodat er een deurtje opent bij invoer van de juiste code. De bedoeling is om een makkelijk herinstelbare opstelling te maken zodat zonder extra gereedschap de initiële situatie kan bekomen worden.
Wanneer 3 keer een foutieve code wordt ingevoerd zal het deurtje ook opengaan maar zal er ook weergegeven worden dat het 'game over' is.

## Detail beschrijving
Initiëel zal de opstelling niet bruikbaar zijn, de LCD zal het 'numpad' weergeven maar zal niet responsief zijn. In deze fase wacht de ESP op de 4 digits die hij door moet gestuurd krijgen van elke proef, de wifi moet dus aanblijven zolang dit niet het geval is en afzonderlijke MQTT signalen kunnen verstuurd worden op dit moment. Eens de 4 digits ontvangen zijn zal de ESP wifi afsluiten aangezien de touchscreen bepaalde pinnen die de wifi gebruikt ook nodig heeft om te kunnen werken. Eenmaal in deze fase is er dus geen terugkeren en is MQTT niet mogelijk.

## Materiaal
* ESP-WROOM-32
* TFT SPI 240x320 touchscreen LCD
* Solenoïde
* 11.1V LiPo batterij
* 12V to 3.3V step down convertor
* 3.3V naar 5V step up convertor
* 12V relay (5V aansturing)
