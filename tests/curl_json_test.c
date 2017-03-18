/****************************************************************************
*                                                                           *
* Test coded by Antoni Gimènez as a part of sat-alert software developed	*
* for Terrassa Ground Station within UPC Space Program.                     *
*                                                           				*
*****************************************************************************
*                                                                           *
* This program is free software; you can redistribute it and/or modify it   *
* under the terms of the GNU General Public License as published by the     *
* Free Software Foundation; either version 2 of the License or any later    *
* version.                                                                  *
*                                                                           *
* This program is distributed in the hope that it will useful, but WITHOUT  *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or     *
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License     *
* for more details.                                                         *
*                                                                           *
*****************************************************************************
* Notice: in order to compile it properly do as follow                      *
*                                                                           *
*   gcc curl_json_test.c -lcurl -ljson-c -o curl_json_test                  *
*                                                                           *
****************************************************************************/


#include <stdio.h>
#include <curl/curl.h>
#include <json-c/json.h>

int main()
{
    CURL *curl;
    CURLcode res;
 
    
    /***************
    *    JSON-C    *
    ***************/

    //First we create a JSON object
    json_object * jObj = json_object_new_object();

    //Create a string JSON element
    json_object *jString = json_object_new_string("TEST MESSAGE");

    //Include the string element jString in json_object as "text":"TEST MESSAGE"
    json_object_object_add(jObj,"text", jString);



    /***************
    *     CURL     *
    ***************/

    //Start Curl with the parameter that starts all the modules,only needed once
    curl_global_init(CURL_GLOBAL_ALL);

    //Create an easy handle curl    
    curl = curl_easy_init();


    if(curl) {

        //Define Curl propierties:

        curl_easy_setopt(curl, CURLOPT_URL, "https://hooks.slack.com/services/XXXXXXXXXXXXXXXXXXXXXXXXXXXXX");

        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_object_to_json_string(jObj));


        //Perform the request, res will get the return code 
        res = curl_easy_perform(curl);


        //Check for errors           
        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        

        //Cleanup the easy handle curl created
        curl_easy_cleanup(curl);
    }


    //Cleanup the global curl, cleanup all modules
    curl_global_cleanup();


    return 0;
}
