package dev.dezva.re4bridge;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
class NativeActionQueueTests {
 @Test void arrowThenConfirmWaitsForAcknowledgementAndReleaseInterval(){
  var q=new NativeActionQueue();q.offer(7,5,0);q.offer(7,6,0);
  var right=q.next(7,0,0,0);assertEquals(5,right.kind());assertEquals(1,right.id());
  assertEquals(1,q.next(7,100_000_000L,7,1).id());
  assertNull(q.next(7,150_000_000L,7,1));assertNull(q.next(7,175_000_000L,7,1));
  var enter=q.next(7,180_000_000L,7,1);assertEquals(6,enter.kind());assertEquals(2,enter.id());
  assertNull(q.next(7,330_000_000L,7,2));
 }
 @Test void stoppedOrExpiredActionsDoNotLeakAcrossMenusOrGenerations(){
  var q=new NativeActionQueue();q.offer(7,6,0);assertNull(q.next(8,0,7,1));q.clear();assertNull(q.next(7,0,0,0));
  q.offer(7,6,0);assertEquals(2,q.next(7,0,0,0).id());assertNull(q.next(7,500_000_000L,0,0));
  q.offer(7,6,0);assertNull(q.next(7,1_000_000_000L,0,0));
  q.offer(8,4,1_000_000_001L);assertEquals(1,q.next(8,1_000_000_001L,7,999).id());
 }
}
