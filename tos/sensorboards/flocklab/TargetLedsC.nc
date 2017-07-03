// $Id: TargetLedsC.nc 2375 2013-04-19 08:35:27Z walserc $
/**
 *
 * The TinyOS LEDs abstraction for LEDs on the FlockLab target node.
 *
 * @author Roman Lim
 */

configuration TargetLedsC {
  provides interface Leds;
}
implementation {
  components TargetLedsP as LedsP, TargetPlatformLedsC as PlatformLedsC;

  Leds = LedsP;

  LedsP.Init <- PlatformLedsC.Init;
  LedsP.Led0 -> PlatformLedsC.Led0;
  LedsP.Led1 -> PlatformLedsC.Led1;
  LedsP.Led2 -> PlatformLedsC.Led2;
}
