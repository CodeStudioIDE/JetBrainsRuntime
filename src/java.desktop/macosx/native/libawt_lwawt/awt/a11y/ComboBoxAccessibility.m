/*
 * Copyright (c) 2022, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2022, JetBrains s.r.o.. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

#import "ComboBoxAccessibility.h"
#import "../JavaAccessibilityUtilities.h"
#import "ThreadUtilities.h"
#import "JNIUtilities.h"

static jclass sjc_CAccessibility = NULL;

static jmethodID sjm_getAccessibleName = NULL;
#define GET_ACCESSIBLENAME_METHOD_RETURN(ret) \
    GET_CACCESSIBILITY_CLASS_RETURN(ret); \
    GET_STATIC_METHOD_RETURN(sjm_getAccessibleName, sjc_CAccessibility, "getAccessibleName", \
                     "(Ljavax/accessibility/Accessible;Ljava/awt/Component;)Ljava/lang/String;", ret);

@implementation ComboBoxAccessibility

- (CommonComponentAccessibility *)accessibleSelection
{
    JNIEnv *env = [ThreadUtilities getJNIEnv];
    GET_CACCESSIBILITY_CLASS_RETURN(nil);
    DECLARE_STATIC_METHOD_RETURN(sjm_getAccessibleComboboxValue, sjc_CAccessibility, "getAccessibleComboboxValue", "(Ljavax/accessibility/Accessible;Ljava/awt/Component;)Ljavax/accessibility/Accessible;", nil);
    jobject axSelectedChild = (*env)->CallStaticObjectMethod(env, sjc_CAccessibility, sjm_getAccessibleComboboxValue, fAccessible, fComponent);
    CHECK_EXCEPTION();
    if (axSelectedChild == NULL) {
        return nil;
    }
    if (value != nil) {
        [value release];
        value = nil;
    }
    value = [CommonComponentAccessibility createWithAccessible:axSelectedChild withEnv:env withView:fView];
    return value;
}

// NSAccessibilityElement protocol methods

- (id)accessibilityValue
{
    JNIEnv *env = [ThreadUtilities getJNIEnv];
    BOOL expanded = isExpanded(env, [self axContextWithEnv:env], fComponent);
    if (expanded) {
        return nil;
    }
    if (!expanded &&
        (value == nil)) {
        [self accessibleSelection];
    }

    return [value accessibilityLabel];
}

- (NSArray *)accessibilitySelectedChildren
{
    return [NSArray arrayWithObject:[self accessibleSelection]];
}

- (void)dealloc
{
    if (value != nil) {
        [value release];
        value = nil;
    }
    [super dealloc];
}

@end
