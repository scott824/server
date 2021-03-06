/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
// vim: ft=cpp:expandtab:ts=8:sw=4:softtabstop=4:
#ident "$Id$"
/*======
This file is part of PerconaFT.


Copyright (c) 2006, 2015, Percona and/or its affiliates. All rights reserved.

    PerconaFT is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License, version 2,
    as published by the Free Software Foundation.

    PerconaFT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PerconaFT.  If not, see <http://www.gnu.org/licenses/>.

----------------------------------------

    PerconaFT is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License, version 3,
    as published by the Free Software Foundation.

    PerconaFT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with PerconaFT.  If not, see <http://www.gnu.org/licenses/>.
======= */

#ident "Copyright (c) 2006, 2015, Percona and/or its affiliates. All rights reserved."

#include "test.h"

static int int64_key_cmp(DB *db UU(), const DBT *a, const DBT *b) {
    int64_t x = *(int64_t *)a->data;
    int64_t y = *(int64_t *)b->data;

    if (x < y)
        return -1;
    if (x > y)
        return 1;
    return 0;
}

static void test_prefetch_read(int fd, FT_HANDLE UU(ft), FT ft_h) {
    int r;
    FT_CURSOR XMALLOC(cursor);
    FTNODE dn = NULL;
    PAIR_ATTR attr;

    // first test that prefetching everything should work
    memset(&cursor->range_lock_left_key, 0, sizeof(DBT));
    memset(&cursor->range_lock_right_key, 0, sizeof(DBT));
    cursor->left_is_neg_infty = true;
    cursor->right_is_pos_infty = true;
    cursor->disable_prefetching = false;

    ftnode_fetch_extra bfe;

    // quick test to see that we have the right behavior when we set
    // disable_prefetching to true
    cursor->disable_prefetching = true;
    bfe.create_for_prefetch(ft_h, cursor);
    FTNODE_DISK_DATA ndd = NULL;
    r = toku_deserialize_ftnode_from(
        fd, make_blocknum(20), 0 /*pass zero for hash*/, &dn, &ndd, &bfe);
    invariant(r == 0);
    invariant(dn->n_children == 3);
    invariant(BP_STATE(dn, 0) == PT_ON_DISK);
    invariant(BP_STATE(dn, 1) == PT_ON_DISK);
    invariant(BP_STATE(dn, 2) == PT_ON_DISK);
    r = toku_ftnode_pf_callback(dn, ndd, &bfe, fd, &attr);
    invariant(BP_STATE(dn, 0) == PT_ON_DISK);
    invariant(BP_STATE(dn, 1) == PT_ON_DISK);
    invariant(BP_STATE(dn, 2) == PT_ON_DISK);
    bfe.destroy();
    toku_ftnode_free(&dn);
    toku_free(ndd);

    // now enable prefetching again
    cursor->disable_prefetching = false;

    bfe.create_for_prefetch(ft_h, cursor);
    r = toku_deserialize_ftnode_from(
        fd, make_blocknum(20), 0 /*pass zero for hash*/, &dn, &ndd, &bfe);
    invariant(r == 0);
    invariant(dn->n_children == 3);
    invariant(BP_STATE(dn, 0) == PT_AVAIL);
    invariant(BP_STATE(dn, 1) == PT_AVAIL);
    invariant(BP_STATE(dn, 2) == PT_AVAIL);
    toku_ftnode_pe_callback(
        dn, make_pair_attr(0xffffffff), ft_h, def_pe_finalize_impl, nullptr);
    invariant(BP_STATE(dn, 0) == PT_COMPRESSED);
    invariant(BP_STATE(dn, 1) == PT_COMPRESSED);
    invariant(BP_STATE(dn, 2) == PT_COMPRESSED);
    r = toku_ftnode_pf_callback(dn, ndd, &bfe, fd, &attr);
    invariant(BP_STATE(dn, 0) == PT_AVAIL);
    invariant(BP_STATE(dn, 1) == PT_AVAIL);
    invariant(BP_STATE(dn, 2) == PT_AVAIL);
    bfe.destroy();
    toku_ftnode_free(&dn);
    toku_free(ndd);

    uint64_t left_key = 150;
    toku_fill_dbt(&cursor->range_lock_left_key, &left_key, sizeof(uint64_t));
    cursor->left_is_neg_infty = false;
    bfe.create_for_prefetch(ft_h, cursor);
    r = toku_deserialize_ftnode_from(
        fd, make_blocknum(20), 0 /*pass zero for hash*/, &dn, &ndd, &bfe);
    invariant(r == 0);
    invariant(dn->n_children == 3);
    invariant(BP_STATE(dn, 0) == PT_ON_DISK);
    invariant(BP_STATE(dn, 1) == PT_AVAIL);
    invariant(BP_STATE(dn, 2) == PT_AVAIL);
    toku_ftnode_pe_callback(
        dn, make_pair_attr(0xffffffff), ft_h, def_pe_finalize_impl, nullptr);
    invariant(BP_STATE(dn, 0) == PT_ON_DISK);
    invariant(BP_STATE(dn, 1) == PT_COMPRESSED);
    invariant(BP_STATE(dn, 2) == PT_COMPRESSED);
    r = toku_ftnode_pf_callback(dn, ndd, &bfe, fd, &attr);
    invariant(BP_STATE(dn, 0) == PT_ON_DISK);
    invariant(BP_STATE(dn, 1) == PT_AVAIL);
    invariant(BP_STATE(dn, 2) == PT_AVAIL);
    bfe.destroy();
    toku_ftnode_free(&dn);
    toku_free(ndd);

    uint64_t right_key = 151;
    toku_fill_dbt(&cursor->range_lock_right_key, &right_key, sizeof(uint64_t));
    cursor->right_is_pos_infty = false;
    bfe.create_for_prefetch(ft_h, cursor);
    r = toku_deserialize_ftnode_from(
        fd, make_blocknum(20), 0 /*pass zero for hash*/, &dn, &ndd, &bfe);
    invariant(r == 0);
    invariant(dn->n_children == 3);
    invariant(BP_STATE(dn, 0) == PT_ON_DISK);
    invariant(BP_STATE(dn, 1) == PT_AVAIL);
    invariant(BP_STATE(dn, 2) == PT_ON_DISK);
    toku_ftnode_pe_callback(
        dn, make_pair_attr(0xffffffff), ft_h, def_pe_finalize_impl, nullptr);
    invariant(BP_STATE(dn, 0) == PT_ON_DISK);
    invariant(BP_STATE(dn, 1) == PT_COMPRESSED);
    invariant(BP_STATE(dn, 2) == PT_ON_DISK);
    r = toku_ftnode_pf_callback(dn, ndd, &bfe, fd, &attr);
    invariant(BP_STATE(dn, 0) == PT_ON_DISK);
    invariant(BP_STATE(dn, 1) == PT_AVAIL);
    invariant(BP_STATE(dn, 2) == PT_ON_DISK);
    bfe.destroy();
    toku_ftnode_free(&dn);
    toku_free(ndd);

    left_key = 100000;
    right_key = 100000;
    bfe.create_for_prefetch(ft_h, cursor);
    r = toku_deserialize_ftnode_from(
        fd, make_blocknum(20), 0 /*pass zero for hash*/, &dn, &ndd, &bfe);
    invariant(r == 0);
    invariant(dn->n_children == 3);
    invariant(BP_STATE(dn, 0) == PT_ON_DISK);
    invariant(BP_STATE(dn, 1) == PT_ON_DISK);
    invariant(BP_STATE(dn, 2) == PT_AVAIL);
    toku_ftnode_pe_callback(
        dn, make_pair_attr(0xffffffff), ft_h, def_pe_finalize_impl, nullptr);
    invariant(BP_STATE(dn, 0) == PT_ON_DISK);
    invariant(BP_STATE(dn, 1) == PT_ON_DISK);
    invariant(BP_STATE(dn, 2) == PT_COMPRESSED);
    r = toku_ftnode_pf_callback(dn, ndd, &bfe, fd, &attr);
    invariant(BP_STATE(dn, 0) == PT_ON_DISK);
    invariant(BP_STATE(dn, 1) == PT_ON_DISK);
    invariant(BP_STATE(dn, 2) == PT_AVAIL);
    bfe.destroy();
    toku_free(ndd);
    toku_ftnode_free(&dn);

    left_key = 100;
    right_key = 100;
    bfe.create_for_prefetch(ft_h, cursor);
    r = toku_deserialize_ftnode_from(
        fd, make_blocknum(20), 0 /*pass zero for hash*/, &dn, &ndd, &bfe);
    invariant(r == 0);
    invariant(dn->n_children == 3);
    invariant(BP_STATE(dn, 0) == PT_AVAIL);
    invariant(BP_STATE(dn, 1) == PT_ON_DISK);
    invariant(BP_STATE(dn, 2) == PT_ON_DISK);
    toku_ftnode_pe_callback(
        dn, make_pair_attr(0xffffffff), ft_h, def_pe_finalize_impl, nullptr);
    invariant(BP_STATE(dn, 0) == PT_COMPRESSED);
    invariant(BP_STATE(dn, 1) == PT_ON_DISK);
    invariant(BP_STATE(dn, 2) == PT_ON_DISK);
    r = toku_ftnode_pf_callback(dn, ndd, &bfe, fd, &attr);
    invariant(BP_STATE(dn, 0) == PT_AVAIL);
    invariant(BP_STATE(dn, 1) == PT_ON_DISK);
    invariant(BP_STATE(dn, 2) == PT_ON_DISK);
    bfe.destroy();
    toku_ftnode_free(&dn);
    toku_free(ndd);

    toku_free(cursor);
}

static void test_subset_read(int fd, FT_HANDLE UU(ft), FT ft_h) {
    int r;
    FT_CURSOR XMALLOC(cursor);
    FTNODE dn = NULL;
    FTNODE_DISK_DATA ndd = NULL;
    PAIR_ATTR attr;

    // first test that prefetching everything should work
    memset(&cursor->range_lock_left_key, 0, sizeof(DBT));
    memset(&cursor->range_lock_right_key, 0, sizeof(DBT));
    cursor->left_is_neg_infty = true;
    cursor->right_is_pos_infty = true;

    uint64_t left_key = 150;
    uint64_t right_key = 151;
    DBT left, right;
    toku_fill_dbt(&left, &left_key, sizeof(left_key));
    toku_fill_dbt(&right, &right_key, sizeof(right_key));

    ftnode_fetch_extra bfe;
    bfe.create_for_subset_read(
        ft_h, NULL, &left, &right, false, false, false, false);

    // fake the childnum to read
    // set disable_prefetching ON
    bfe.child_to_read = 2;
    bfe.disable_prefetching = true;
    r = toku_deserialize_ftnode_from(
        fd, make_blocknum(20), 0 /*pass zero for hash*/, &dn, &ndd, &bfe);
    invariant(r == 0);
    invariant(dn->n_children == 3);
    invariant(BP_STATE(dn, 0) == PT_ON_DISK);
    invariant(BP_STATE(dn, 1) == PT_ON_DISK);
    invariant(BP_STATE(dn, 2) == PT_AVAIL);
    // need to call this twice because we had a subset read before, that touched
    // the clock
    toku_ftnode_pe_callback(
        dn, make_pair_attr(0xffffffff), ft_h, def_pe_finalize_impl, nullptr);
    invariant(BP_STATE(dn, 0) == PT_ON_DISK);
    invariant(BP_STATE(dn, 1) == PT_ON_DISK);
    invariant(BP_STATE(dn, 2) == PT_AVAIL);
    toku_ftnode_pe_callback(
        dn, make_pair_attr(0xffffffff), ft_h, def_pe_finalize_impl, nullptr);
    invariant(BP_STATE(dn, 0) == PT_ON_DISK);
    invariant(BP_STATE(dn, 1) == PT_ON_DISK);
    invariant(BP_STATE(dn, 2) == PT_COMPRESSED);
    r = toku_ftnode_pf_callback(dn, ndd, &bfe, fd, &attr);
    invariant(BP_STATE(dn, 0) == PT_ON_DISK);
    invariant(BP_STATE(dn, 1) == PT_ON_DISK);
    invariant(BP_STATE(dn, 2) == PT_AVAIL);
    toku_ftnode_free(&dn);
    toku_free(ndd);

    // fake the childnum to read
    bfe.child_to_read = 2;
    bfe.disable_prefetching = false;
    r = toku_deserialize_ftnode_from(
        fd, make_blocknum(20), 0 /*pass zero for hash*/, &dn, &ndd, &bfe);
    invariant(r == 0);
    invariant(dn->n_children == 3);
    invariant(BP_STATE(dn, 0) == PT_ON_DISK);
    invariant(BP_STATE(dn, 1) == PT_AVAIL);
    invariant(BP_STATE(dn, 2) == PT_AVAIL);
    // need to call this twice because we had a subset read before, that touched
    // the clock
    toku_ftnode_pe_callback(
        dn, make_pair_attr(0xffffffff), ft_h, def_pe_finalize_impl, nullptr);
    invariant(BP_STATE(dn, 0) == PT_ON_DISK);
    invariant(BP_STATE(dn, 1) == PT_COMPRESSED);
    invariant(BP_STATE(dn, 2) == PT_AVAIL);
    toku_ftnode_pe_callback(
        dn, make_pair_attr(0xffffffff), ft_h, def_pe_finalize_impl, nullptr);
    invariant(BP_STATE(dn, 0) == PT_ON_DISK);
    invariant(BP_STATE(dn, 1) == PT_COMPRESSED);
    invariant(BP_STATE(dn, 2) == PT_COMPRESSED);
    r = toku_ftnode_pf_callback(dn, ndd, &bfe, fd, &attr);
    invariant(BP_STATE(dn, 0) == PT_ON_DISK);
    invariant(BP_STATE(dn, 1) == PT_AVAIL);
    invariant(BP_STATE(dn, 2) == PT_AVAIL);
    toku_ftnode_free(&dn);
    toku_free(ndd);

    // fake the childnum to read
    bfe.child_to_read = 0;
    r = toku_deserialize_ftnode_from(
        fd, make_blocknum(20), 0 /*pass zero for hash*/, &dn, &ndd, &bfe);
    invariant(r == 0);
    invariant(dn->n_children == 3);
    invariant(BP_STATE(dn, 0) == PT_AVAIL);
    invariant(BP_STATE(dn, 1) == PT_AVAIL);
    invariant(BP_STATE(dn, 2) == PT_ON_DISK);
    // need to call this twice because we had a subset read before, that touched
    // the clock
    toku_ftnode_pe_callback(
        dn, make_pair_attr(0xffffffff), ft_h, def_pe_finalize_impl, nullptr);
    invariant(BP_STATE(dn, 0) == PT_AVAIL);
    invariant(BP_STATE(dn, 1) == PT_COMPRESSED);
    invariant(BP_STATE(dn, 2) == PT_ON_DISK);
    toku_ftnode_pe_callback(
        dn, make_pair_attr(0xffffffff), ft_h, def_pe_finalize_impl, nullptr);
    invariant(BP_STATE(dn, 0) == PT_COMPRESSED);
    invariant(BP_STATE(dn, 1) == PT_COMPRESSED);
    invariant(BP_STATE(dn, 2) == PT_ON_DISK);
    r = toku_ftnode_pf_callback(dn, ndd, &bfe, fd, &attr);
    invariant(BP_STATE(dn, 0) == PT_AVAIL);
    invariant(BP_STATE(dn, 1) == PT_AVAIL);
    invariant(BP_STATE(dn, 2) == PT_ON_DISK);
    toku_ftnode_free(&dn);
    toku_free(ndd);

    toku_free(cursor);
}

static void test_prefetching(void) {
    //    struct ft_handle source_ft;
    struct ftnode sn;

    int fd = open(TOKU_TEST_FILENAME,
                  O_RDWR | O_CREAT | O_BINARY,
                  S_IRWXU | S_IRWXG | S_IRWXO);
    invariant(fd >= 0);

    int r;

    //    source_ft.fd=fd;
    sn.max_msn_applied_to_node_on_disk.msn = 0;
    sn.flags = 0x11223344;
    sn.blocknum.b = 20;
    sn.layout_version = FT_LAYOUT_VERSION;
    sn.layout_version_original = FT_LAYOUT_VERSION;
    sn.height = 1;
    sn.n_children = 3;
    sn.dirty = 1;
    sn.oldest_referenced_xid_known = TXNID_NONE;

    uint64_t key1 = 100;
    uint64_t key2 = 200;

    MALLOC_N(sn.n_children, sn.bp);
    DBT pivotkeys[2];
    toku_fill_dbt(&pivotkeys[0], &key1, sizeof(key1));
    toku_fill_dbt(&pivotkeys[1], &key2, sizeof(key2));
    sn.pivotkeys.create_from_dbts(pivotkeys, 2);
    BP_BLOCKNUM(&sn, 0).b = 30;
    BP_BLOCKNUM(&sn, 1).b = 35;
    BP_BLOCKNUM(&sn, 2).b = 40;
    BP_STATE(&sn, 0) = PT_AVAIL;
    BP_STATE(&sn, 1) = PT_AVAIL;
    BP_STATE(&sn, 2) = PT_AVAIL;
    set_BNC(&sn, 0, toku_create_empty_nl());
    set_BNC(&sn, 1, toku_create_empty_nl());
    set_BNC(&sn, 2, toku_create_empty_nl());
    // Create XIDS
    XIDS xids_0 = toku_xids_get_root_xids();
    XIDS xids_123;
    XIDS xids_234;
    r = toku_xids_create_child(xids_0, &xids_123, (TXNID)123);
    CKERR(r);
    r = toku_xids_create_child(xids_123, &xids_234, (TXNID)234);
    CKERR(r);

    // data in the buffers does not matter in this test
    // Cleanup:
    toku_xids_destroy(&xids_0);
    toku_xids_destroy(&xids_123);
    toku_xids_destroy(&xids_234);

    FT_HANDLE XMALLOC(ft);
    FT XCALLOC(ft_h);
    toku_ft_init(ft_h,
                 make_blocknum(0),
                 ZERO_LSN,
                 TXNID_NONE,
                 4 * 1024 * 1024,
                 128 * 1024,
                 TOKU_DEFAULT_COMPRESSION_METHOD,
                 16);
    ft_h->cmp.create(int64_key_cmp, nullptr);
    ft->ft = ft_h;
    ft_h->blocktable.create();
    {
        int r_truncate = ftruncate(fd, 0);
        CKERR(r_truncate);
    }
    // Want to use block #20
    BLOCKNUM b = make_blocknum(0);
    while (b.b < 20) {
        ft_h->blocktable.allocate_blocknum(&b, ft_h);
    }
    invariant(b.b == 20);

    {
        DISKOFF offset;
        DISKOFF size;
        ft_h->blocktable.realloc_on_disk(b, 100, &offset, ft_h, fd, false);
        invariant(offset ==
               (DISKOFF)BlockAllocator::BLOCK_ALLOCATOR_TOTAL_HEADER_RESERVE);

        ft_h->blocktable.translate_blocknum_to_offset_size(b, &offset, &size);
        invariant(offset ==
               (DISKOFF)BlockAllocator::BLOCK_ALLOCATOR_TOTAL_HEADER_RESERVE);
        invariant(size == 100);
    }
    FTNODE_DISK_DATA ndd = NULL;
    r = toku_serialize_ftnode_to(
        fd, make_blocknum(20), &sn, &ndd, true, ft->ft, false);
    invariant(r == 0);

    test_prefetch_read(fd, ft, ft_h);
    test_subset_read(fd, ft, ft_h);

    toku_destroy_ftnode_internals(&sn);

    ft_h->blocktable.block_free(
        BlockAllocator::BLOCK_ALLOCATOR_TOTAL_HEADER_RESERVE, 100);
    ft_h->blocktable.destroy();
    ft_h->cmp.destroy();
    toku_free(ft_h->h);
    toku_free(ft_h);
    toku_free(ft);
    toku_free(ndd);

    r = close(fd);
    invariant(r != -1);
}

int test_main(int argc __attribute__((__unused__)),
              const char *argv[] __attribute__((__unused__))) {
    test_prefetching();

    return 0;
}
