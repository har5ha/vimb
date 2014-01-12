/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2014 Daniel Carl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */

#include "config.h"
#include "js.h"


static gboolean evaluate_string(JSContextRef ctx, const char *script,
    const char *file, JSValueRef *result);

/**
 * Run scripts out of given file in the given frame.
 */
gboolean js_eval_file(WebKitWebFrame *frame, const char *file)
{
    char *js = NULL, *value = NULL;
    GError *error = NULL;

    if (g_file_test(file, G_FILE_TEST_IS_REGULAR)
        && g_file_get_contents(file, &js, NULL, &error)
    ) {
        gboolean success = js_eval(frame, js, file, &value);
        if (!success) {
            fprintf(stderr, "%s", value);
        }
        g_free(value);
        g_free(js);

        return success;
    }

    return false;
}

/**
 * Evaluates given string as script and return if this call succeed or not.
 * On success the given **value pointer is filles with the returned string,
 * else with the exception message. In both cases this must be freed by the
 * caller if no longer used.
 */
gboolean js_eval(WebKitWebFrame *frame, const char *script, const char *file,
    char **value)
{
    gboolean   success;
    JSValueRef result = NULL;
    JSContextRef ctx  = webkit_web_frame_get_global_context(frame);

    success = evaluate_string(ctx, script, file, &result);
    *value  = js_ref_to_string(ctx, result);

    return success;
}

/**
 * Creates a JavaScript object in contect of given frame.
 */
JSObjectRef js_create_object(WebKitWebFrame *frame, const char *script)
{
    JSValueRef result = NULL, exc = NULL;
    JSObjectRef object;
    JSContextRef ctx  = webkit_web_frame_get_global_context(frame);

    if (!evaluate_string(ctx, script, NULL, &result)) {
        return NULL;
    }

    object = JSValueToObject(ctx, result, &exc);
    if (exc) {
        return NULL;
    }
    JSValueProtect(ctx, result);

    return object;
}

/**
 * Calls a function on object and returns the result as newly allocates
 * string.
 * Returned string must be freed after use.
 */
char* js_object_call_function(JSContextRef ctx, JSObjectRef obj,
    const char *func, int count, JSValueRef params[])
{
    JSValueRef  js_ret, function;
    JSObjectRef function_object;
    JSStringRef js_func = NULL;
    char *value;

    if (!obj) {
        return NULL;
    }

    js_func = JSStringCreateWithUTF8CString(func);
    if (!JSObjectHasProperty(ctx, obj, js_func)) {
        JSStringRelease(js_func);

        return NULL;
    }

    function        = JSObjectGetProperty(ctx, obj, js_func, NULL);
    function_object = JSValueToObject(ctx, function, NULL);
    js_ret          = JSObjectCallAsFunction(ctx, function_object, NULL, count, params, NULL);
    JSStringRelease(js_func);

    value = js_ref_to_string(ctx, js_ret);

    return value;
}

/**
 * Retrune a new allocates string for given valeu reference.
 * String must be freed if not used anymore.
 */
char* js_ref_to_string(JSContextRef ctx, JSValueRef ref)
{
    char *string;
    JSStringRef str_ref = JSValueToStringCopy(ctx, ref, NULL);
    size_t len          = JSStringGetMaximumUTF8CStringSize(str_ref);

    string = g_new0(char, len);
    JSStringGetUTF8CString(str_ref, string, len);
    JSStringRelease(str_ref);

    return string;
}

/**
 * Retrieves a values reference for given string.
 */
JSValueRef js_string_to_ref(JSContextRef ctx, const char *string)
{
    JSStringRef js = JSStringCreateWithUTF8CString(string);
    JSValueRef ref = JSValueMakeString(ctx, js);
    JSStringRelease(js);
    return ref;
}

/**
 * Runs a string as JavaScript and returns if the call succeed.
 * In case the call succeed, the given *result is filled with the result
 * value, else with the value reference of the exception.
 */
static gboolean evaluate_string(JSContextRef ctx, const char *script,
    const char *file, JSValueRef *result)
{
    JSStringRef js_str, js_file;
    JSValueRef exc = NULL, res = NULL;

    js_str  = JSStringCreateWithUTF8CString(script);
    js_file = JSStringCreateWithUTF8CString(file);

    res = JSEvaluateScript(ctx, js_str, JSContextGetGlobalObject(ctx), js_file, 0, &exc);
    JSStringRelease(js_file);
    JSStringRelease(js_str);

    if (exc) {
        *result = exc;
        return false;
    }

    *result = res;
    return true;
}