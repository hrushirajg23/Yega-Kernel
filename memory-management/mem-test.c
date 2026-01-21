/*/1* */
/* * STATUS: all tests are running completely */
/* * one after another . */
/* * Exception */
/* * 8th test is failing sometimes due to excess memory, so use low object sizes that */ 
/* * are convenient for your memory, currently I have only checked for */
/* * 16, 32 and 64MB memory. */
/* *  But works when alone tested. */
/* *1/ */


/*#define TEST_MAGIC_ALLOC  0xAAAABBBB */
/*#define TEST_MAGIC_FREE   0xDEADBEEF */

/*static inline void write_pattern(void *p, size_t size, uint32_t pattern) */
/*{ */
/*    uint32_t *w = (uint32_t *)p; */
/*    size_t n = size / 4; */
/*    while (n--) */
/*        *w++ = pattern; */
/*} */

/*static inline int verify_pattern(void *p, size_t size, uint32_t pattern) */
/*{ */
/*    uint32_t *w = (uint32_t *)p; */
/*    size_t n = size / 4; */
/*    while (n--) { */
/*        if (*w != pattern) */
/*            return 0; */
/*        w++; */
/*    } */
/*    return 1; */
/*} */

/*/1************************************** */
/* * TEST 1: Basic alloc/free */
/* **************************************/ 
/*void test_basic_alloc_free(void) */
/*{ */
/*    printk("\n[TEST 1] Basic alloc/free\n"); */

/*    void *a = kmalloc(48, 0); */
/*    void *b = kmalloc(48, 0); */

/*    if (!a || !b) */
/*        printk("FAIL: basic alloc returned NULL\n"); */

/*    kfree(a); */
/*    kfree(b); */

/*    printk("[OK] test_basic_alloc_free\n"); */
/*} */

/*/1************************************** */
/* * TEST 2: Many allocs (grow + shrink) */
/* **************************************/ 
/*void test_many_allocs(void) */
/*{ */
/*    printk("\n[TEST 2] Many allocs\n"); */

/*    void *ptrs[1024]; */
/*    int i; */

/*    for (i = 0; i < 1024; i++) { */
/*        ptrs[i] = kmalloc(32, 0); */
/*        if (!ptrs[i]) { */
/*            printk("FAIL: alloc failed at i=%d\n", i); */
/*            return; */
/*        } */
/*    } */

/*    for (i = 0; i < 1024; i++) */
/*        kfree(ptrs[i]); */

/*    printk("[OK] test_many_allocs\n"); */
/*} */

/*/1************************************** */
/* * TEST 3: Pattern test (detect memory corruption) */
/* **************************************/
/*void test_pattern_integrity(void) */
/*{ */
/*    printk("\n[TEST 3] Pattern integrity test\n"); */

/*    void *p = kmalloc(64, 0); */
/*    if (!p) { */
/*        printk("FAIL: kmalloc returned NULL\n"); */
/*        return; */
/*    } */

/*    write_pattern(p, 64, TEST_MAGIC_ALLOC); */

/*    if (!verify_pattern(p, 64, TEST_MAGIC_ALLOC)) */
/*        printk("FAIL: corruption BEFORE free!\n"); */

/*    kfree(p); */

/*    printk("[OK] test_pattern_integrity\n"); */
/*} */

/*/1************************************** */
/* * TEST 4: Stress test (10000 allocations) */
/* **************************************/ 
/*void test_stress_allocs(void) */
/*{ */
/*    printk("\n[TEST 4] Stress test 10000 alloc/free\n"); */

/*    for (int i = 0; i < 10000; i++) { */
/*        void *p = kmalloc(96, 0); */
/*        if (!p) { */
/*            printk("FAIL at iter %d\n", i); */
/*            return; */
/*        } */
/*        kfree(p); */
/*    } */

/*    printk("[OK] test_stress_allocs\n"); */
/*} */

/*/1************************************** */
/* * TEST 5: Cross cache consistency */
/* **************************************/
/*void test_cross_cache(void) */
/*{ */
/*    printk("\n[TEST 5] Cross cache consistency\n"); */

/*    void *a = kmalloc(24, 0);   // size-32 cache */
/*    void *b = kmalloc(500, 0);  // size-512 cache */
/*    void *c = kmalloc(8000, 0); // size-8192 cache */

/*    if (!a || !b || !c) */
/*        printk("FAIL: one of allocs failed\n"); */

/*    kfree(a); */
/*    kfree(b); */
/*    kfree(c); */

/*    printk("[OK] test_cross_cache\n"); */
/*} */

/*/1************************************** */
/* * TEST 6: Reuse test (free then allocate again) */
/* **************************************/ 
/*void test_reuse(void) */
/*{ */
/*    printk("\n[TEST 6] Object reuse test\n"); */

/*    void *p1 = kmalloc(64, 0); */
/*    kfree(p1); */

/*    void *p2 = kmalloc(64, 0); */

/*    if (p1 != p2) */
/*        printk("[WARN] allocator did NOT reuse the same block\n"); */
/*    else */
/*        printk("[OK] block reuse verified\n"); */
/*} */

/*/1************************************** */
/* * TEST 7: Slab list invariants */
/* * (Slabs must be either in free, partial or full — never both) */
/* **************************************/ 
/*void test_slab_invariants(void) */
/*{ */
/*    printk("\n[TEST 7] Slab list invariants\n"); */

/*    // just allocate enough to force multiple slabs */
/*    for (int i = 0; i < 300; i++) */
/*        kmalloc(64, 0); */

/*    printk("Manually inspect lists via your display_kmemlist() calls.\n"); */
/*    printk("[OK] invariant test executed\n"); */
/*} */

/*/1************************************** */
/* * TEST 8: Very small and very large allocations */
/* **************************************/ 
/*void test_extreme_sizes(void) */
/*{ */
/*    printk("\n[TEST 8] Extreme sizes test\n"); */

/*    void *a = kmalloc(1, 0); */
/*    void *b = kmalloc(131072, 0); // maximum slab size */

/*    if (!a || !b) */
/*        printk("FAIL: alloc returned NULL\n"); */

/*    kfree(a); */
/*    kfree(b); */

/*    printk("[OK] test_extreme_sizes\n"); */
/*} */

/*/1************************************** */
/* * TEST 9: Alloc/free random sizes */
/* **************************************/ 
/*void test_random_churn(void) */
/*{ */
/*    printk("\n[TEST 9] Random churn test\n"); */

/*    for (int i = 0; i < 500; i++) { */
/*        size_t sz = (i * 37) % 130000 + 1; */
/*        void *p = kmalloc(sz, 0); */
/*        if (!p) { */
/*            printk("FAIL: churn alloc failed at iter %d\n", i); */
/*            return; */
/*        } */
/*        kfree(p); */
/*    } */

/*    printk("[OK] test_random_churn\n"); */
/*} */


/*/1************************************** */
/* * RUN ALL TESTS */
/* **************************************/ 
/*void run_slab_tests(void) */
/*{ */
/*    printk("\n========================\n"); */
/*    printk("   RUNNING SLAB TESTS   \n"); */
/*    printk("========================\n"); */

/*    /1* test_basic_alloc_free(); *1/ */
/*    /1* test_many_allocs(); *1/ */
/*    /1* test_pattern_integrity(); *1/ */
/*    /1* test_stress_allocs(); *1/ */
/*    /1* test_cross_cache(); *1/ */
/*    /1* test_reuse(); *1/ */
/*    /1* test_slab_invariants(); *1/ */
/*    /1* test_extreme_sizes(); *1/ */
/*    test_random_churn(); */

/*    printk("\n========================\n"); */
/*    printk("   SLAB TESTS COMPLETE  \n"); */
/*    printk("========================\n"); */
/*} */

