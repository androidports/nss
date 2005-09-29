/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Netscape security libraries.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1994-2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Sun Microsystems
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
/*
 * test_buildchain_resourcelimits.c
 *
 * Test BuildChain User Checker function
 *
 */

#include "testutil.h"
#include "testutil_nss.h"

#define PKIX_TESTUSERCHECKER_TYPE (PKIX_NUMTYPES+30)

#if 0
extern char *pkix_pl_PK11ConfigDir = "../../nist_pkits/certs_and_crls";
#endif

void *plContext = NULL;
static PKIX_UInt32 numUserCheckerCalled = 0;

void printUsage(void){
        (void) printf("\nUSAGE:\ttest_buildchain_resourcelimits [ENE|EE] "
                    "<trustedCert> <targetCert> <certStoreDirectory>\n\n");
        (void) printf
                ("Builds a chain of certificates between "
                "<trustedCert> and <targetCert>\n"
                "using the certs and CRLs in <certStoreDirectory>.\n"
                "If ENE is specified, then an Error is Not Expected.\n"
                "If EE is specified, an Error is Expected.\n");
}

PKIX_PL_Cert *
createDirCert(
        char *dirName,
        char *certFile,
        void *plContext)
{
        PKIX_PL_Cert *cert = NULL;
        char *certPathName = NULL;
        PKIX_UInt32 certFileLen;
        PKIX_UInt32 dirNameLen;

        PKIX_TEST_STD_VARS();

        certFileLen = PL_strlen(certFile);
        dirNameLen = PL_strlen(dirName);

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_PL_Malloc
                                    (dirNameLen + certFileLen + 2,
                                    (void **)&certPathName,
                                    plContext));

        PL_strcpy(certPathName, dirName);
        PL_strcat(certPathName, "/");
        PL_strcat(certPathName, certFile);
        printf("certPathName = %s\n", certPathName);
        cert = createCert(certPathName, plContext);

cleanup:

        PKIX_PL_Free(certPathName, plContext);

        PKIX_TEST_RETURN();

        return (cert);

}

void Test_BuildResult(
        PKIX_BuildParams *buildParams,
        PKIX_Boolean testValid,
        PKIX_UInt32 chainLength,
        PKIX_List *expectedCerts,
        void *plContext)
{
        PKIX_PL_Cert *cert = NULL;
        PKIX_List *certs = NULL;
        PKIX_CertChain *chain = NULL;
        PKIX_PL_String *actualCertsString = NULL;
        PKIX_PL_String *expectedCertsString = NULL;
        PKIX_BuildResult *buildResult = NULL;
        PKIX_Boolean result;
        PKIX_Boolean supportForward = PKIX_FALSE;
        PKIX_UInt32 numCerts, i;
        char *asciiResult = NULL;
        char *actualCertsAscii = NULL;
        char *expectedCertsAscii = NULL;

        PKIX_TEST_STD_VARS();

        pkixTestErrorResult =
                PKIX_BuildChain(buildParams, &buildResult, plContext);

        if (testValid == PKIX_TRUE) { /* ENE */
                if (pkixTestErrorResult){
                        (void) printf("UNEXPECTED RESULT RECEIVED!\n");
                } else {
                        (void) printf("EXPECTED RESULT RECEIVED!\n");
                        PKIX_TEST_DECREF_BC(pkixTestErrorResult);
                }
        } else { /* EE */
                if (pkixTestErrorResult){
                        (void) printf("EXPECTED RESULT RECEIVED!\n");
                        PKIX_TEST_DECREF_BC(pkixTestErrorResult);
                } else {
                        testError("UNEXPECTED RESULT RECEIVED");
                }
        }

        if (buildResult){

                PKIX_TEST_EXPECT_NO_ERROR
                        (PKIX_BuildResult_GetCertChain
                        (buildResult, &chain, NULL));

                PKIX_TEST_EXPECT_NO_ERROR
                        (PKIX_CertChain_GetCertificates
                        (chain, &certs, plContext));

                PKIX_TEST_EXPECT_NO_ERROR
                        (PKIX_List_GetLength(certs, &numCerts, plContext));

                printf("\n");

                for (i = 0; i < numCerts; i++){
                        PKIX_TEST_EXPECT_NO_ERROR
                                (PKIX_List_GetItem
                                (certs,
                                i,
                                (PKIX_PL_Object**)&cert,
                                plContext));

                        asciiResult = PKIX_Cert2ASCII(cert);

                        printf("CERT[%d]:\n%s\n", i, asciiResult);

                        PKIX_TEST_EXPECT_NO_ERROR
                                (PKIX_PL_Free(asciiResult, plContext));
                        asciiResult = NULL;

                        PKIX_TEST_DECREF_BC(cert);
                }

                PKIX_TEST_EXPECT_NO_ERROR
                        (PKIX_PL_Object_Equals
                        ((PKIX_PL_Object*)certs,
                        (PKIX_PL_Object*)expectedCerts,
                        &result,
                        plContext));

                if (!result){
                        testError("BUILT CERTCHAIN IS "
                                    "NOT THE ONE THAT WAS EXPECTED");

                        PKIX_TEST_EXPECT_NO_ERROR
                                (PKIX_PL_Object_ToString
                                ((PKIX_PL_Object *)certs,
                                &actualCertsString,
                                plContext));

                        actualCertsAscii = PKIX_String2ASCII
                                (actualCertsString, plContext);
                        if (actualCertsAscii == NULL){
                                pkixTestErrorMsg = "PKIX_String2ASCII Failed";
                                goto cleanup;
                        }

                        PKIX_TEST_EXPECT_NO_ERROR
                                (PKIX_PL_Object_ToString
                                ((PKIX_PL_Object *)expectedCerts,
                                &expectedCertsString,
                                plContext));

                        expectedCertsAscii = PKIX_String2ASCII
                                (expectedCertsString, plContext);
                        if (expectedCertsAscii == NULL){
                                pkixTestErrorMsg = "PKIX_String2ASCII Failed";
                                goto cleanup;
                        }

                        (void) printf("Actual value:\t%s\n", actualCertsAscii);
                        (void) printf("Expected value:\t%s\n",
                                        expectedCertsAscii);

                        if (chainLength - 1 != numUserCheckerCalled) {
		                pkixTestErrorMsg =
                                    "PKIX user defined checker not called";
                        }

                        goto cleanup;
                }

        }
cleanup:

        PKIX_PL_Free(asciiResult, plContext);
        PKIX_PL_Free(actualCertsAscii, plContext);
        PKIX_PL_Free(expectedCertsAscii, plContext);
        PKIX_TEST_DECREF_AC(cert);
        PKIX_TEST_DECREF_AC(certs);
        PKIX_TEST_DECREF_AC(chain);
        PKIX_TEST_DECREF_AC(buildResult);
        PKIX_TEST_DECREF_AC(actualCertsString);
        PKIX_TEST_DECREF_AC(expectedCertsString);

        PKIX_TEST_RETURN();

}

int main(int argc, char *argv[])
{
        PKIX_BuildParams *buildParams = NULL;
        PKIX_ComCertSelParams *certSelParams = NULL;
        PKIX_CertSelector *certSelector = NULL;
        PKIX_TrustAnchor *anchor = NULL;
        PKIX_List *anchors = NULL;
        PKIX_ProcessingParams *procParams = NULL;
        PKIX_CertChainChecker *checker = NULL;
        PKIX_ResourceLimits *resourceLimits = NULL;
        char *dirName = NULL;
        PKIX_PL_String *dirNameString = NULL;
        PKIX_PL_Cert *trustedCert = NULL;
        PKIX_PL_Cert *targetCert = NULL;
        PKIX_PL_Cert *dirCert = NULL;
        PKIX_UInt32 actualMinorVersion, j, k, chainLength;
        PKIX_CertStore *certStore = NULL;
        PKIX_List *certStores = NULL;
        PKIX_List *expectedCerts = NULL;
        PKIX_Boolean testValid = PKIX_TRUE;

        PKIX_TEST_STD_VARS();

        startTests("BuildChain_ResourceLimits");

        /* This must precede the call to PKIX_Initialize! */



        PKIX_TEST_EXPECT_NO_ERROR(PKIX_Initialize
                                    (PKIX_MAJOR_VERSION,
                                    PKIX_MINOR_VERSION,
                                    PKIX_MINOR_VERSION,
                                    &actualMinorVersion,
                                    plContext));
        if (argc < 4){
                printUsage();
                return (0);
        }

        j = 0;

        PKIX_TEST_NSSCONTEXT_SETUP(0x10, argv[1], NULL, &plContext);

        /* ENE = expect no error; EE = expect error */
        if (PORT_Strcmp(argv[2+j], "ENE") == 0) {
                testValid = PKIX_TRUE;
        } else if (PORT_Strcmp(argv[2+j], "EE") == 0) {
                testValid = PKIX_FALSE;
        } else {
                printUsage();
                return (0);
        }

        subTest(argv[1+j]);

        dirName = argv[3+j];

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_List_Create(&expectedCerts, plContext));

        chainLength = argc - j - 4;

        for (k = 0; k < chainLength; k++){

                dirCert = createDirCert(dirName, argv[4+k+j], plContext);

                if (k == (chainLength - 1)){
                        PKIX_TEST_EXPECT_NO_ERROR
                                (PKIX_PL_Object_IncRef
                                ((PKIX_PL_Object *)dirCert, plContext));
                        trustedCert = dirCert;
                } else {

                        PKIX_TEST_EXPECT_NO_ERROR
                                (PKIX_List_AppendItem
                                (expectedCerts,
                                (PKIX_PL_Object *)dirCert,
                                plContext));

                        if (k == 0){
                                PKIX_TEST_EXPECT_NO_ERROR
                                        (PKIX_PL_Object_IncRef
                                        ((PKIX_PL_Object *)dirCert,
                                        plContext));
                                targetCert = dirCert;
                        }
                }

                PKIX_TEST_DECREF_BC(dirCert);
        }

        /* create processing params with list of trust anchors */

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_TrustAnchor_CreateWithCert
                                    (trustedCert, &anchor, plContext));
        PKIX_TEST_EXPECT_NO_ERROR(PKIX_List_Create(&anchors, plContext));
        PKIX_TEST_EXPECT_NO_ERROR
                (PKIX_List_AppendItem
                (anchors, (PKIX_PL_Object *)anchor, plContext));
        PKIX_TEST_EXPECT_NO_ERROR(PKIX_ProcessingParams_Create
                                    (anchors, &procParams, plContext));

        /* create CertSelector with target certificate in params */

        PKIX_TEST_EXPECT_NO_ERROR
                (PKIX_ComCertSelParams_Create(&certSelParams, plContext));

        PKIX_TEST_EXPECT_NO_ERROR
                (PKIX_ComCertSelParams_SetCertificate
                (certSelParams, targetCert, plContext));

        PKIX_TEST_EXPECT_NO_ERROR
                (PKIX_CertSelector_Create
                (NULL, NULL, &certSelector, plContext));

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_CertSelector_SetCommonCertSelectorParams
                                (certSelector, certSelParams, plContext));

        PKIX_TEST_EXPECT_NO_ERROR
                (PKIX_ProcessingParams_SetTargetCertConstraints
                (procParams, certSelector, plContext));

        /* create CertStores */

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_PL_String_Create
                                    (PKIX_ESCASCII,
                                    dirName,
                                    0,
                                    &dirNameString,
                                    plContext));

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_PL_CollectionCertStore_Create
                                    (dirNameString, &certStore, plContext));

#if 0
        PKIX_TEST_EXPECT_NO_ERROR(PKIX_PL_Pk11CertStore_Create
                                    (&certStore, plContext));
#endif


        PKIX_TEST_EXPECT_NO_ERROR(PKIX_List_Create(&certStores, plContext));

        PKIX_TEST_EXPECT_NO_ERROR
                (PKIX_List_AppendItem
                (certStores, (PKIX_PL_Object *)certStore, plContext));

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_ProcessingParams_SetCertStores
                                    (procParams, certStores, plContext));

        /* set resource limits */

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_ResourceLimits_Create
                (&resourceLimits, plContext));

        /* need longer time when running dbx for memory leak checking */
        PKIX_TEST_EXPECT_NO_ERROR(PKIX_ResourceLimits_SetMaxTime
                (resourceLimits, 60, plContext));

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_ResourceLimits_SetMaxFanout
                (resourceLimits, 2, plContext));

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_ResourceLimits_SetMaxDepth
                (resourceLimits, 2, plContext));

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_ProcessingParams_SetResourceLimits
                                    (procParams, resourceLimits, plContext));

        /* create build params with processing params */
        PKIX_TEST_EXPECT_NO_ERROR(PKIX_BuildParams_Create
                                    (procParams, &buildParams, plContext));

        /* build cert chain using build params and return buildResult */

        subTest("Testing ResourceLimits MaxFanout & MaxDepth - <pass>");
        Test_BuildResult
                (buildParams,
                testValid,
                chainLength,
                expectedCerts,
                plContext);

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_ResourceLimits_SetMaxFanout
                (resourceLimits, 1, plContext));

        subTest("Testing ResourceLimits MaxFanout - <fail>");
        Test_BuildResult
                (buildParams,
                PKIX_FALSE,
                chainLength,
                expectedCerts,
                plContext);

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_ResourceLimits_SetMaxFanout
                (resourceLimits, 2, plContext));
        PKIX_TEST_EXPECT_NO_ERROR(PKIX_ResourceLimits_SetMaxDepth
                (resourceLimits, 1, plContext));

        subTest("Testing ResourceLimits MaxDepth - <fail>");
        Test_BuildResult
                (buildParams,
                PKIX_FALSE,
                chainLength,
                expectedCerts,
                plContext);

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_ResourceLimits_SetMaxFanout
                (resourceLimits, 0, plContext));
        PKIX_TEST_EXPECT_NO_ERROR(PKIX_ResourceLimits_SetMaxDepth
                (resourceLimits, 0, plContext));
        PKIX_TEST_EXPECT_NO_ERROR(PKIX_ResourceLimits_SetMaxTime
                (resourceLimits, 0, plContext));

        subTest("Testing ResourceLimits No checking - <pass>");
        Test_BuildResult
                (buildParams,
                testValid,
                chainLength,
                expectedCerts,
                plContext);

cleanup:

        PKIX_TEST_DECREF_AC(expectedCerts);
        PKIX_TEST_DECREF_AC(certStore);
        PKIX_TEST_DECREF_AC(certStores);
        PKIX_TEST_DECREF_AC(dirNameString);
        PKIX_TEST_DECREF_AC(trustedCert);
        PKIX_TEST_DECREF_AC(targetCert);
        PKIX_TEST_DECREF_AC(anchor);
        PKIX_TEST_DECREF_AC(anchors);
        PKIX_TEST_DECREF_AC(procParams);
        PKIX_TEST_DECREF_AC(certSelParams);
        PKIX_TEST_DECREF_AC(certSelector);
        PKIX_TEST_DECREF_AC(buildParams);
        PKIX_TEST_DECREF_AC(checker);
        PKIX_TEST_DECREF_AC(resourceLimits);

        PKIX_TEST_RETURN();

        PKIX_Shutdown(plContext);

        endTests("BuildChain_UserChecker");

        return (0);

}
