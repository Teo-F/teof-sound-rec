#include "rec-identity.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    sr_identity s={0};sr_file_identity a,b,c,bad;
    sr_format f={44100,2,24,6};assert(!sr_identity_begin(&s));
    char path[]="/media/usb/sdb1/ALPHATHETA REC/REC001.WAV";
    assert(!sr_identity_stage(&s,123,f,path,&a));path[1]='x';
    assert(s.pending.path[1]=='m');assert(!sr_identity_matches(&s,&a));
    assert(!sr_identity_activate(&s,&a));
    assert(sr_identity_stage(&s,123,f,"/duplicate.wav",&bad)==-1);
    assert(!sr_identity_stage(&s,456,f,"/media/usb/sdb1/REC002.WAV",&b));
    assert(sr_identity_matches(&s,&a));assert(!sr_identity_matches(&s,&b));
    assert(sr_identity_stage(&s,0,f,"/bad",&bad)==-1);
    assert(s.pending.birth==b.birth); /* failed creation leaves known pending */
    assert(!sr_identity_stage(&s,789,f,"/media/usb/sdb1/REC003.WAV",&c));
    assert(sr_identity_activate(&s,&b)==-1); /* superseded pending */
    assert(!sr_identity_activate(&s,&c));assert(!sr_identity_matches(&s,&a));
    assert(sr_identity_retire(&s,&a)==-1); /* old destructor after promotion */
    assert(sr_identity_matches(&s,&c));
    assert(!sr_identity_stage(&s,888,f,"/media/usb/sdb1/REC004.WAV",&b));
    assert(!sr_identity_retire(&s,&b)); /* discard pending, preserve active */
    assert(sr_identity_matches(&s,&c));assert(sr_identity_activate(&s,&b)==-1);
    sr_identity_end(&s);assert(!sr_identity_matches(&s,&c));
    assert(!sr_identity_begin(&s));
    assert(!sr_identity_stage(&s,789,f,"/media/usb/sdb1/REC003.WAV",&b));
    assert(!sr_identity_activate(&s,&b));assert(!sr_identity_matches(&s,&c));
    char large[SR_PATH_MAX+1];memset(large,'a',sizeof large);large[0]='/';large[SR_PATH_MAX]=0;
    assert(sr_identity_stage(&s,1,f,large,&bad)==-1);
    assert(sr_identity_stage(&s,1,f,"REC004.WAV",&bad)==-1);
    puts("identity: staged/active, copied path, replaced pending, failure, stop and reused-pointer generations passed");
}
