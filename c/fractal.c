                                                        /*
                                                        sn*/
                                                       typedef
                                                        float
                                              f;   f R,a,C,t,A,l      =
                                               60;v()          {return(
                                             R>999               ) ?0:
                                            255u*                 (sin(
                                           0.1f *                 R +a++
                           ) /2.000+      0.500f)                  ;}main
                            (       ){   for (                     f y =
         /**/            0;y         ++ < l;                       puts
(""),usleep(60000))for(f x            =0;x                       ++<l
         /**/            *2;         printf                       ("%c"
                           "["     "4"   "8;2"                     ";%d"
                         "" ";%d;%dm "    ,27,v(),               v(),v()
                                           ))for(                a=4,C=t
                                            =R=0;C              *C+t*t<
                                             4&&R++              <1000
                                               ;A=C*C         -t* t+ x/
                                              .7/  l-2.1,t=C*t*2+     y/
                                                       .35/l
                                                       -1.4,C
                                                        =A);
                                                          }
