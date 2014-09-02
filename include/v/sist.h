    /**  Disk persistence.
    **/

         void u2_sist_mkdir_chkpt_dir();

         void u2_sist_get_pier_dirstr(c3_c * str_w, int len_w);
         void u2_sist_get_lockfile_filestr(c3_c * str_w, int len_w);
         void u2_sist_get_pill_filestr(c3_c * str_w, int len_w);
         void u2_sist_get_egz_filestr(c3_c* buf_c, int len_c);
         void u2_sist_get_egz_quick_dirstr(c3_c * str_w, int len_w);
         void u2_sist_get_egz_quick_filestr(c3_c * str_w, int len_w, c3_d sequence_d, c3_y msgtype_y);
         void u2_sist_get_chkpt_filestr(c3_c* fil_c, c3_c* suf_c, c3_c* buf_c, int len_c);

      /* u2_sist_boot(): restore or create pier from disk.
      */
        void
        u2_sist_boot(void);

      /* u2_sist_put(): moronic key-value store put.
      **
      ** u2_sist_put will do its best to associate the passed key with
      ** the passed value in a way that will persist across process
      ** restarts. It will probably do so by writing a file named for
      ** the key with contents identical to the value. To rely on it
      ** for anything heavy-duty would be a mistake.
      **
      ** Why would we even have something like this? Because sometimes
      ** we need to maintain files completely independently of the
      ** noun state.
      */
        void
        u2_sist_put(const c3_c* key_c, const c3_y* val_y, size_t siz_i);

      /* u2_sist_nil(): moronic key-value store rm.
      **
      ** Does its best to expunge all records on the given key. Has
      ** no effect if the key doesn't exist.
      */
        void
        u2_sist_nil(const c3_c* key_c);

      /* u2_sist_has(): moronic key-value store existence check.
      **
      ** Returns the byte length of the value previously stored via
      ** u2_sist_put, or -1 if it couldn't find one.
      */
        ssize_t
        u2_sist_has(const c3_c* key_c);

      /* u2_sist_get(): moronic key-value store get.
      **
      ** u2_sist_get is the mirror of u2_sist_put. It writes to val_y,
      ** which had better be at least as big as the return value from
      ** u2_sist_has, the value that you previously put.
      **
      ** Needless to say, u2_sist_get crashes if it can't find your
      ** value.
      */
        void
        u2_sist_get(const c3_c* key_c, c3_y* val_y);


