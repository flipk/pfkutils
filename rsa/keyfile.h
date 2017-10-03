
/* return 0 if wrote ok */
int  putkeys      ( mpz_t n, mpz_t d, mpz_t e, int bytes );

/* return 0 if read ok */
int  getkeys_pub  ( mpz_t n, mpz_t e, int *bytes );
int  getkeys_priv ( mpz_t n, mpz_t d, int *bytes );
