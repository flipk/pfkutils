
/* 
 * this program generates passwords.
 * even if you read this and realize that my passwords
 * have certain characteristics, realize also that the
 * resulting keyspace is still much too large to crack
 * empirically.
 *
 * password criteria enforced by motorola:
 *
 *   - at least 8 chars long
 *   - at least one A-Z
 *   - at least one a-z
 *   - at least one 0-9
 *   - at least one $-_#
 *   - must start with letter
 *
 * additional criteria that i enforce:
 *
 *   - exactly 8 chars long
 *   - every letter alternates hands on keyboard (i'm lazy)
 *   - never more than 2 digits (i'm lazy)
 *   - only one letter is capitalized (i'm lazy)
 *   - only right-hand letters are capitalized
 *    (i only use left shift key, cuz i'm lazy)
 *   - never use '6' .. on some keyboards its on the right
 *    hand and on others its on the left.
 *
 * oh, and it does all this in 259 characters.
 */

genpw_main(a,h,g,i,l,c,p,r){char w[9];srand(time(w[8]=0));for(;!r|h^1|!g|g>2;){l=(p=rand()%8)%2;for(h=g=r=i=0;i<8;i++,l^=1){(a=(c="yuiophjklnm7890---__qwertasdfgzxcvb12345"[rand()%20+l*20])>96)&&!i?r++:c==45||c==95?h++:c<58?g++:0;w[i]=i==p?!a?r=0:c-32:c;}}puts(w);}

#if 0
/*
 * a = isalpha flag
 * i = index into password
 * l = left/right index into key; either 0 or 1
 * c = current character
 * p = index of letter to be capitalized
 * r = first char is a letter
 * g = digit counter
 * h = dash counter
 * w = output password
 */

main(a,h,g,i,l,c,p,r)
{
    char w[9];
    srand(time(w[8]=0));
    for ( ; !r | h^1 | !g | g>2; )
    {
        l = (p=rand()%8) %2;
        for (h=g=r=i=0; i < 8; i++, l^=1 )
        {
            (a=
             (c="yuiophjklnm7890---__qwertasdfgzxcvb12345"
              [rand()%20+l*20]
                 )>96) && 
                !i ? r++ :
                c==45 || c==95 ? h++ :
                c < 58 ? g++ : 0;
            w[i]= i==p ? !a ? r=0 : c-32 : c;
        }
    }
    puts(w);
}

main(a,h,g,i,l,c,p,r){char w[9];srandom(time(w[8]=0));for(;!r|h^1|!g|g>2;){l=(p=random()%8)%2;for(h=g=r=i=0;i<8;i++,l^=1){(a=(c="yuiophjklnm7890---__qwertasdfgzxcvb12345"[random()%20+l*20])>96)&&!i?r++:c==45||c==95?h++:c<58?g++:0;w[i]=i==p?!a?r=0:c-32:c;}}puts(w);}


#endif
