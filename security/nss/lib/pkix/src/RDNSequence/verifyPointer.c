/* 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Netscape security libraries.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1994-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

#ifdef DEBUG
static const char CVS_ID[] = "@(#) $Source$ $Revision$ $Date$ $Name$";
#endif /* DEBUG */

#ifndef PKIXM_H
#include "pkixm.h"
#endif /* PKIXM_H */

#ifdef DEBUG

extern const NSSError NSS_ERROR_INTERNAL_ERROR;

static nssPointerTracker pkix_rdnseq_pointer_tracker;

/*
 * nss_pkix_RDNSequence_add_pointer
 *
 * This method is only present in debug builds.
 *
 * This module-private routine adds an NSSPKIXRDNSequence pointer to
 * the internal pointer-tracker.  This routine should only be used 
 * by the NSSPKIX module.  This routine returns a PRStatus value; 
 * upon error it will place an error on the error stack and return 
 * PR_FAILURE.
 *
 * The error may be one of the following values:
 *  NSS_ERROR_NO_MEMORY
 *  NSS_ERROR_INTERNAL_ERROR
 *
 * Return value:
 *  PR_SUCCESS upon success
 *  PR_FAILURE upon failure
 */

NSS_IMPLEMENT PRStatus
nss_pkix_RDNSequence_add_pointer
(
  const NSSPKIXRDNSequence *p
)
{
  PRStatus rv;

  rv = nssPointerTracker_initialize(&pkix_rdnseq_pointer_tracker);
  if( PR_SUCCESS != rv ) {
    return rv;
  }

  rv = nssPointerTracker_add(&pkix_rdnseq_pointer_tracker, p);
  if( PR_SUCCESS != rv ) {
    NSSError e = NSS_GetError();
    if( NSS_ERROR_NO_MEMORY != e ) {
      nss_SetError(NSS_ERROR_INTERNAL_ERROR);
    }

    return rv;
  }

  rv = nssArena_registerDestructor(arena,
         nss_pkix_RDNSequence_remove_pointer, p);
  if( PR_SUCCESS != rv ) {
    (void)nss_pkix_RDNSequence_remove_pointer(p);
    return rv;
  }

#ifdef NSSDEBUG
  {
    NSSPKIXRelativeDistinguishedName *r;

    for( r = p->rdns; *r; r++ ) {
      if( PR_SUCCESS != nss_pkix_RelativeDistinguishedName_add_pointer(*r) ) {
        (void)nss_pkix_RDNSequence_remove_pointer(p);
        return PR_FAILURE;
      }
    }
  }
#endif /* NSSDEBUG */

  return PR_SUCCESS;
}

/*
 * nss_pkix_RDNSequence_remove_pointer
 *
 * This method is only present in debug builds.
 *
 * This module-private routine removes a valid NSSPKIXRDNSequence
 * pointer from the internal pointer-tracker.  This routine should 
 * only be used by the NSSPKIX module.  This routine returns a 
 * PRStatus value; upon error it will place an error on the error 
 * stack and return PR_FAILURE.
 *
 * The error may be one of the following values:
 *  NSS_ERROR_INTERNAL_ERROR
 *
 * Return value:
 *  PR_SUCCESS upon success
 *  PR_FAILURE upon failure
 */

NSS_IMPLEMENT PRStatus
nss_pkix_RDNSequence_remove_pointer
(
  const NSSPKIXRDNSequence *p
)
{
  PRStatus rv;

#ifdef NSSDEBUG
  {
    NSSPKIXRelativeDistinguishedName *r;

    for( r = p->rdns; *r; r++ ) {
      (void)nss_pkix_RelativeDistinguishedName_remove_pointer(*r);
    }
  }
#endif /* NSSDEBUG */

  rv = nssPointerTracker_remove(&pkix_rdnseq_pointer_tracker, p);
  if( PR_SUCCESS != rv ) {
    nss_SetError(NSS_ERROR_INTERNAL_ERROR);
  }

  /*
   * nssArena_deregisterDestructor(p->arena,
   *   nss_pkix_RDNSequence_remove_pointer, p);
   */

  return rv;
}

/*
 * nssPKIXRDNSequence_verifyPointer
 *
 * This method is only present in debug builds.
 *
 * If the specified pointer is a valid pointer to an NSSPKIXRDNSequence
 * object, this routine will return PR_SUCCESS.  Otherwise, it will 
 * put an error on the error stack and return PR_FAILURE.
 *
 * The error may be one of the following values:
 *  NSS_ERROR_INVALID_PKIX_ATTRIBUTE
 *
 * Return value:
 *  PR_SUCCESS if the pointer is valid
 *  PR_FAILURE if it isn't
 */

NSS_IMPLEMENT PRStatus
nssPKIXRDNSequence_verifyPointer
(
  NSSPKIXRDNSequence *p
)
{
  PRStatus rv;

  rv = nssPointerTracker_initialize(&pkix_rdnseq_pointer_tracker);
  if( PR_SUCCESS != rv ) {
    nss_SetError(NSS_ERROR_INVALID_PKIX_ATTRIBUTE);
    return PR_FAILURE;
  }

  rv = nssPointerTracker_verify(&pkix_rdnseq_pointer_tracker, p);
  if( PR_SUCCESS != rv ) {
    nss_SetError(NSS_ERROR_INVALID_PKIX_ATTRIBUTE);
    return PR_FAILURE;
  }

  return PR_SUCCESS;
}

#endif /* DEBUG */

