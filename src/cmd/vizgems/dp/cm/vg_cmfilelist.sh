files=(
    customer=(
        name=customer
        summary="The Customer to ID Mapping File"
        description="This file maps customer names to unique ids"

        group="vg_att_admin"

        location=$VGCM_CUSTFILE
        filefuns=vg_cm_customer
        fileprint=vg_cmdefprint
        filecheck=vg_cmdefcheck
        fileedit=vg_cmdefedit
        listsize=20

        fields=(
            field0=(
                name=id
                text="Customer ID"
                type=text
                info="the unique id for the customer"
                size=60
                search=yes
            )
            field1=(
                name=name
                text="Customer Name"
                type=text
                info="the customer name"
                size=60
                search=yes
            )
            field2=(
                name=nodename
                text="Formatted Name"
                type=text
                info="the location name with optional newlines"
                size=60
                search=yes
            )
        )
    )

    business=(
        name=business
        summary="The Business File"
        description="This file contains the list of all businesses"

        group="vg_att_admin"

        location=$VGCM_BIZFILE
        filefuns=vg_cm_business
        fileprint=vg_cmdefprint
        filecheck=vg_cmdefcheck
        fileedit=vg_cmdefedit
        listsize=20

        fields=(
            field0=(
                name=id
                text="Business ID"
                type=text
                info="the unique id for the business, e.g. hosting"
                size=60
                search=yes
            )
            field1=(
                name=name
                text="Business Name"
                type=text
                info="the business name, e.g. HOSTING"
                size=60
                search=yes
            )
            field2=(
                name=nodename
                text="Formatted Name"
                type=text
                info="the business name with optional newlines, e.g. HOSTING"
                size=60
                search=yes
            )
        )
    )

    location=(
        name=location
        summary="The Location File"
        description="This file maps location names to longitude / latitude"

        group="vg_att_admin"

        location=$VGCM_LOCFILE
        filefuns=vg_cm_location
        fileprint=vg_cmdefprint
        filecheck=vg_cmdefcheck
        fileedit=vg_cmdefedit
        listsize=20

        fieldmode=multi:0
        fields=(
            field0=(
                name=rtype
                text="Record Type"
                type=choice
                defval="node|Node;edge|Edge;map|Map"
                info="specifies the record type"
                search=yes
            )
        )
        multifields=(
            choice0=(
                field0=(
                    name=level
                    text="Level"
                    type=text
                    info="the asset level"
                    size=60
                    helper=vg_cmdefvaluehelper
                    search=yes
                )
                field1=(
                    name=id
                    text="Asset ID"
                    type=text
                    info="the asset id"
                    size=60
                    search=yes
                )
                field2=(
                    name=key
                    text="Key"
                    type=text
                    info="the property key"
                    size=60
                    search=yes
                )
                field3=(
                    name=val
                    text="Value"
                    type=text
                    info="the property value"
                    size=60
                    search=yes
                )
            )
            choice1=(
                field0=(
                    name=level1
                    text="Level1"
                    type=text
                    info="the asset level for asset 1"
                    size=60
                    search=yes
                )
                field1=(
                    name=id1
                    text="Asset ID1"
                    type=text
                    info="the asset id for asset 1"
                    size=60
                    search=yes
                )
                field2=(
                    name=level2
                    text="Level2"
                    type=text
                    info="the asset level for asset 2"
                    size=60
                    search=yes
                )
                field3=(
                    name=id2
                    text="Asset ID2"
                    type=text
                    info="the asset level for asset 2"
                    size=60
                    search=yes
                )
                field4=(
                    name=key
                    text="Key"
                    type=text
                    info="the property key"
                    size=60
                    search=yes
                )
                field5=(
                    name=val
                    text="Value"
                    type=text
                    info="the property val"
                    size=60
                    search=yes
                )
            )
            choice2=(
                field0=(
                    name=level1
                    text="Level1"
                    type=text
                    info="the asset level for asset 1"
                    size=60
                    search=yes
                )
                field1=(
                    name=id1
                    text="Asset ID1"
                    type=text
                    info="the asset id for asset 1"
                    size=60
                    search=yes
                )
                field2=(
                    name=level2
                    text="Level2"
                    type=text
                    info="the asset level for asset 2"
                    size=60
                    search=yes
                )
                field3=(
                    name=id2
                    text="Asset ID2"
                    type=text
                    info="the asset level for asset 2"
                    size=60
                    search=yes
                )
                field4=(
                    name=key
                    text="Key"
                    type=text
                    info="the property key"
                    size=60
                    search=yes
                )
                field5=(
                    name=val
                    text="Value"
                    type=text
                    info="the property value"
                    size=60
                    search=yes
                )
            )
        )
    )

    type=(
        name=type
        summary="The Type File"
        description="This file contains information on all asset types"

        group="vg_att_admin"

        location=$VGCM_TYPEFILE
        filefuns=vg_cm_type
        fileprint=vg_cmdefprint
        filecheck=vg_cmdefcheck
        fileedit=vg_cmdefedit
        listsize=20

        fields=(
            field0=(
                name=id
                text="Type ID"
                type=text
                info="the unique id for the type, e.g. idc--web"
                size=60
                search=yes
            )
            field1=(
                name=name
                text="Short Type Name"
                type=text
                info="the short type name, e.g. WEB"
                size=60
                search=yes
            )
            field2=(
                name=nodename
                text="Full Type Name"
                type=text
                info="the full type name / description, e.g. WEB - WEB SERVER"
                size=60
                search=yes
            )
        )
    )

    view=(
        name=view
        summary="The View Inventory Files"
        description="These files contain all the data to construct the views"

        group="vg_att_admin"
        accessmode=byid
        admingroup="vg_att_admin"

        location=$VGCM_VIEWDIR
        locationmode=dir
        locationre='*-inv.txt'
        filefuns=vg_cm_view
        filelist=vg_cmdeflist
        fileprint=vg_cmdefprint
        filecheck=vg_cmdefcheck
        fileedit=vg_cmdefedit
        listsize=20

        fieldmode=multi:0
        fields=(
            field0=(
                name=rtype
                text="Record Type"
                type=choice
                defval="node|Node;edge|Edge;map|Map"
                info="specifies the record type"
                search=yes
            )
        )
        multifields=(
            choice0=(
                field0=(
                    name=level
                    text="Level"
                    type=text
                    info="the asset level"
                    size=60
                    helper=vg_cmdefvaluehelper
                    search=yes
                )
                field1=(
                    name=id
                    text="Asset ID"
                    type=text
                    info="the asset id"
                    size=60
                    search=yes
                )
                field2=(
                    name=key
                    text="Key"
                    type=text
                    info="the property key"
                    size=60
                    search=yes
                )
                field3=(
                    name=val
                    text="Value"
                    type=text
                    info="the property value"
                    size=60
                    search=yes
                )
            )
            choice1=(
                field0=(
                    name=level1
                    text="Level1"
                    type=text
                    info="the asset level for asset 1"
                    size=60
                    search=yes
                )
                field1=(
                    name=id1
                    text="Asset ID1"
                    type=text
                    info="the asset id for asset 1"
                    size=60
                    search=yes
                )
                field2=(
                    name=level2
                    text="Level2"
                    type=text
                    info="the asset level for asset 2"
                    size=60
                    search=yes
                )
                field3=(
                    name=id2
                    text="Asset ID2"
                    type=text
                    info="the asset level for asset 2"
                    size=60
                    search=yes
                )
                field4=(
                    name=key
                    text="Key"
                    type=text
                    info="the property key"
                    size=60
                    search=yes
                )
                field5=(
                    name=val
                    text="Value"
                    type=text
                    info="the property val"
                    size=60
                    search=yes
                )
            )
            choice2=(
                field0=(
                    name=level1
                    text="Level1"
                    type=text
                    info="the asset level for asset 1"
                    size=60
                    search=yes
                )
                field1=(
                    name=id1
                    text="Asset ID1"
                    type=text
                    info="the asset id for asset 1"
                    size=60
                    search=yes
                )
                field2=(
                    name=level2
                    text="Level2"
                    type=text
                    info="the asset level for asset 2"
                    size=60
                    search=yes
                )
                field3=(
                    name=id2
                    text="Asset ID2"
                    type=text
                    info="the asset level for asset 2"
                    size=60
                    search=yes
                )
                field4=(
                    name=key
                    text="Key"
                    type=text
                    info="the property key"
                    size=60
                    search=yes
                )
                field5=(
                    name=val
                    text="Value"
                    type=text
                    info="the property value"
                    size=60
                    search=yes
                )
            )
        )
    )

    scope=(
        name=scope
        summary="The Scope Inventory Files"
        description="These files contain all the data needed to monitor assets"

        group="vg_att_admin"
        accessmode=byid
        admingroup="vg_att_admin"

        location=$VGCM_SCOPEDIR
        locationmode=dir
        locationre='*-inv.txt'
        filefuns=vg_cm_scope
        filelist=vg_cmdeflist
        fileprint=vg_cmdefprint
        filecheck=vg_cmdefcheck
        fileedit=vg_cmdefedit
        listsize=20

        fieldmode=multi:0
        fields=(
            field0=(
                name=rtype
                text="Record Type"
                type=choice
                defval="node|Node;edge|Edge;map|Map"
                info="specifies the record type"
                search=yes
            )
        )
        multifields=(
            choice0=(
                field0=(
                    name=level
                    text="Level"
                    type=text
                    info="the asset level"
                    size=60
                    helper=vg_cmdefvaluehelper
                    search=yes
                )
                field1=(
                    name=id
                    text="Asset ID"
                    type=text
                    info="the asset id"
                    size=60
                    search=yes
                )
                field2=(
                    name=key
                    text="Key"
                    type=text
                    info="the property key"
                    size=60
                    search=yes
                )
                field3=(
                    name=val
                    text="Value"
                    type=text
                    info="the property value"
                    size=60
                    search=yes
                )
            )
            choice1=(
                field0=(
                    name=level1
                    text="Level1"
                    type=text
                    info="the asset level for asset 1"
                    size=60
                    search=yes
                )
                field1=(
                    name=id1
                    text="Asset ID1"
                    type=text
                    info="the asset id for asset 1"
                    size=60
                    search=yes
                )
                field2=(
                    name=level2
                    text="Level2"
                    type=text
                    info="the asset level for asset 2"
                    size=60
                    search=yes
                )
                field3=(
                    name=id2
                    text="Asset ID2"
                    type=text
                    info="the asset level for asset 2"
                    size=60
                    search=yes
                )
                field4=(
                    name=key
                    text="Key"
                    type=text
                    info="the property key"
                    size=60
                    search=yes
                )
                field5=(
                    name=val
                    text="Value"
                    type=text
                    info="the property val"
                    size=60
                    search=yes
                )
            )
            choice2=(
                field0=(
                    name=level1
                    text="Level1"
                    type=text
                    info="the asset level for asset 1"
                    size=60
                    search=yes
                )
                field1=(
                    name=id1
                    text="Asset ID1"
                    type=text
                    info="the asset id for asset 1"
                    size=60
                    search=yes
                )
                field2=(
                    name=level2
                    text="Level2"
                    type=text
                    info="the asset level for asset 2"
                    size=60
                    search=yes
                )
                field3=(
                    name=id2
                    text="Asset ID2"
                    type=text
                    info="the asset level for asset 2"
                    size=60
                    search=yes
                )
                field4=(
                    name=key
                    text="Key"
                    type=text
                    info="the property key"
                    size=60
                    search=yes
                )
                field5=(
                    name=val
                    text="Value"
                    type=text
                    info="the property value"
                    size=60
                    search=yes
                )
            )
        )
    )

    alarmemail=(
        name=alarmemail
        summary="The Alarm Email Filter File"
        description="This file contains records that direct the system to send out email notifications for alarms that match the selection parameters"

        group="vg_att_admin|vg_confeditemail"
        accessmode=byid
        admingroup="vg_att_admin|vg_hevopsview"

        location=$VGCM_ALARMEMAILFILE
        filefuns=vg_cm_alarmemail
        fileprint=vg_cmdefprint
        filecheck=vg_cmdefcheck
        fileedit=vg_cmdefedit
        listsize=10

        fields=(
            field0=(
                name=asset
                text="Asset Name"
                type=text
                info="the full name of an asset or a regular expression describing multiple assets: eg: n01234.* - common regular expressions meta characters allowed: $ ^ [ ] .*"
                helper=vg_cmdefvaluehelper
                search=yes
                size=60
            )
            field1=(
                name=alarmid
                text="Alarm ID"
                type=text
                info="the full alarm id or a regular expression describing multiple alarms - only alarms known to the correlation system have alarm ids"
                search=yes
                size=60
            )
            field2=(
                name=msgtext
                text="Message Text"
                type=text
                info="the full message text or a sub string of it - no regular expressions are recognized except for the use of .*: Threshold.*cycles"
                search=yes
                size=60
            )
            field3=(
                name=tmode
                text="Ticket Mode"
                type=choice
                defval="keep|Keep;drop|Drop;|Any"
                info="the ticketing mode - select a specific mode or Any to match all modes"
                search=yes
            )
            field4=(
                name=severity
                text="Severity"
                type=choice
                defval="critical|Critical;major|Major;minor|Minor;warning|Warning;normal|Normal;|Any"
                info="the severity level - select a specific severity or Any to match all severities"
                search=yes
            )
            field5=(
                name=starttime
                text="Start Time"
                type=text
                info="the start time (HH:MM) of this action"
                helper=vg_cmdefvaluehelper
                search=yes
                size=60
            )
            field6=(
                name=endtime
                text="End Time"
                type=text
                info="the end time (hh:mm) of this action"
                helper=vg_cmdefvaluehelper
                search=yes
                size=60
            )
            field7=(
                name=startdate
                text="Start Date"
                type=text
                info="the start date (YYYY.MM.DD) of this action"
                helper=vg_cmdefvaluehelper
                search=yes
                size=60
            )
            field8=(
                name=enddate
                text="End Date"
                type=text
                info="the end date (YYYY.MM.DD) of this action"
                helper=vg_cmdefvaluehelper
                search=yes
                size=60
            )
            field9=(
                name=repeat
                text="Repeat"
                type=choice
                defval="once|Once;daily|Daily;weekly|Weekly;monthly|Monthly"
                info="the repeat mode - for daily, weekly, and monthly modes, the start and end times specify the initial time interval"
                search=yes
            )
            field10=(
                name=fromaddress
                text="From Address"
                type=text
                info="the From email address"
                search=yes
                size=60
            )
            field11=(
                name=toaddress
                text="To Addresses"
                type=text
                info="the list of To email addresses"
                search=yes
                size=60
            )
            field12=(
                name=style
                text="Email Style"
                type=choice
                defval="html|HTML;text|Text;sms|SMS"
                info="the style of email message - HTML: tabular layout, Text: simple textual layout, SMS: short textual layout appropriate for cell phone text messages"
                search=yes
            )
            field13=(
                name=subject
                text="Email Subject"
                type=text
                info="the subject line of the email message"
                search=yes
                size=60
            )
            field14=(
                name=account
                text="User Account"
                type=textro
                info="the website account that established this email rule"
                search=yes
                size=60
            )
        )
        bulkinsertfield=(
            name=insertfile
            text="Bulk Insert File"
            info="a file with records to insert to the main file - must be in the same format as the main file"
            format="asset|+|alarmid|+|msgtext|+|ticmode|+|sev|+|starttime|+|endtime|+|startdate|+|enddate|+|repeat|+|fromaddress|+|toaddress|+|style|+|subject|+|account"
            example="abc123|+||+|daily messages test|+|keep|+|critical|+|15:42|+|16:42|+|2007.05.17 Wed|+|2007.05.19 Wed|+|daily|+|vizgems@att.com|+|vizgems@att.com|+|sms|+|alarms|+|ek1234"
        )
    )

    filter=(
        name=filter
        summary="The Filter (Blackout) File"
        description="This file contains records that direct the system to suppress, modify, or annotate alarms for alarms that match the selection parameters"

        group="vg_att_admin|vg_confeditfilter"
        accessmode=byid
        admingroup="vg_att_admin|vg_hevopsview"

        location=$VGCM_FILTERFILE
        filefuns=vg_cm_filter
        fileprint=vg_cmdefprint
        filecheck=vg_cmdefcheck
        fileedit=vg_cmdefedit
        listsize=10

        fields=(
            field0=(
                name=asset
                text="Asset Name"
                type=text
                info="the full name of an asset or a regular expression describing multiple assets: eg: n01234.* - common regular expressions meta characters allowed: $ ^ [ ] .*"
                helper=vg_cmdefvaluehelper
                search=yes
                size=60
            )
            field1=(
                name=alarmid
                text="Alarm ID"
                type=text
                info="the full alarm id or a regular expression describing multiple alarms - only alarms known to the correlation system have alarm ids"
                search=yes
                size=60
            )
            field2=(
                name=msgtext
                text="Message Text"
                type=text
                info="the full message text or a sub string of it - no regular expressions are recognized except for the use of .*: Threshold.*cycles"
                search=yes
                size=60
            )
            field3=(
                name=starttime
                text="Start Time"
                type=text
                info="the start time (HH:MM) of this operation"
                helper=vg_cmdefvaluehelper
                search=yes
                size=60
            )
            field4=(
                name=endtime
                text="End Time"
                type=text
                info="the end time (hh:mm) of this operation"
                helper=vg_cmdefvaluehelper
                search=yes
                size=60
            )
            field5=(
                name=startdate
                text="Start Date"
                type=text
                info="the start date (YYYY.MM.DD) of this operation"
                helper=vg_cmdefvaluehelper
                search=yes
                size=60
            )
            field6=(
                name=enddate
                text="End Date"
                type=text
                info="the end date (YYYY.MM.DD) of this operation"
                helper=vg_cmdefvaluehelper
                search=yes
                size=60
            )
            field7=(
                name=repeat
                text="Repeat"
                type=choice
                defval="once|Once;daily|Daily;weekly|Weekly;monthly|Monthly"
                info="the repeat mode - for daily, weekly, and monthly modes, the start and end times specify the initial time interval"
                search=yes
                size=60
            )
            field8=(
                name=tmode
                text="Ticket Mode"
                type=choice
                defval="keep|Keep;drop|Drop"
                info="the ticketing mode - drop drops ticketing, keep sends the alarm through the correlation engine"
                search=yes
            )
            field9=(
                name=smode
                text="Viz Mode"
                type=choice
                defval="keep|Keep;drop|Drop"
                info="the visualization mode - keep will allow the record to appear in query results"
                search=yes
            )
            field10=(
                name=severity
                text="Severity"
                type=choice
                defval="critical|Critical;major|Major;minor|Minor;warning|Warning;normal|Normal;|Unchanged"
                info="the severity level"
                search=yes
            )
            field11=(
                name=comment
                text="Comment"
                type=text
                info="a comment to add to any alarm that matches these parameters"
                search=yes
                size=60
            )
            field12=(
                name=account
                text="User Account"
                type=textro
                info="the website account that established this filter rule"
                search=yes
                size=60
            )
        )
        bulkinsertfield=(
            name=insertfile
            text="Bulk Insert File"
            info="a file with records to insert to the main file - must be in the same format as the main file"
            format="asset|+|alarmid|+|msgtext|+|starttime|+|endtime|+|startdate|+|enddate|+|repeat|+|ticmode|+|vizmode|+|sev|+|comment|+|account"
            example="nbawa029|+||+|daily messages test|+|15:42|+|16:42|+|2004.05.17 Wed|+|2004.05.19 Wed|+|daily|+|drop|+|drop|+||+|testing by ek|+|ek1234"
        )
    )

    favorites=(
        name=favorites

        summary="The Favorites Files"
        description="These files contain the list of user favorites"

        group="vg_*"
        accessmode=byid
        admingroup="vg_att_admin"

        location=$VGCM_FAVORITESDIR
        locationmode=dir
        locationre='*.txt'
        filefuns=vg_cm_favorites
        filelist=vg_cmdeflist
        fileprint=vg_cmdefprint
        filecheck=vg_cmdefcheck
        fileedit=vg_cmdefedit
        listsize=10

        fields=(
            field0=(
                name=label
                text="Query Label"
                type=text
                info="the label that appears in the favorites menu"
                search=yes
                size=60
            )
            field1=(
                name=params
                text="Query Parameters"
                type=text
                info="the query parameters"
                search=yes
                size=60
            )
            field2=(
                name=account
                text="User account"
                type=textro
                info="the website account that established this favorite"
                search=yes
                size=60
            )
        )
    )

    profile=(
        name=profile

        summary="The Profiles File"
        description="This file contains all active statistical profile settings for all monitored assets"

        group="vg_att_admin|vg_confeditprofile"
        accessmode=byid
        admingroup="vg_att_admin|vg_hevopsview"

        location=$VGCM_PROFILEFILE
        filefuns=vg_cm_profile
        fileprint=vg_cmdefprint
        fileprinthelper=vg_cm_profilehelper
        filecheck=vg_cmdefcheck
        fileedit=vg_cmdefedit
        listsize=10

        fields=(
            field0=(
                name=asset
                text="Asset Name"
                type=text
                info="the full name of an asset or a regular expression describing multiple assets: eg: n01234.* - common regular expressions meta characters allowed: $ ^ [ ] .*"
                helper=vg_cmdefvaluehelper
                search=yes
                size=60
            )
            field1=(
                name=pname
                text="Statistic Name"
                type=textro
                info="the name of the statistic to change the profile on"
                helper=vg_cmdefvaluehelper
                search=yes
                size=60
            )
            field2=(
                name=sstatus
                text="Statistic Status"
                type=choice
                defval="keep|Active;drop|Disabled"
                info="enable or disable statistical alarming for this asset"
                search=yes
            )
            field3=(
                name=pvalue
                text="Statistic Profile"
                type=textro
                info="the profile alarm parameters"
                helper=vg_cmdefvaluehelper
                search=yes
                size=60
            )
            field4=(
                name=account
                text="User account"
                type=textro
                info="the website account that established this profile rule"
                search=yes
                size=60
            )
        )
        bulkinsertfield=(
            name=insertfile
            text="Bulk Insert File"
            info="a file with records to insert to the main file - must be in the same format as the main file"
            format="asset|metric|mode|alarmspec|account"
            example="labems01|fs_used|keep|VR:i:5:n::SIGMA:i:6:n::1:3:3:1:3600:CLEAR:5:3:3|swiftadmin"
        )
    )

    threshold=(
        name=threshold

        summary="The Thresholds File"
        description="This file contains all active threshold settings for all monitored assets"

        group="vg_att_admin|vg_confeditthreshold"
        accessmode=byid
        admingroup="vg_att_admin|vg_hevopsview"

        location=$VGCM_THRESHOLDFILE
        filefuns=vg_cm_threshold
        fileprint=vg_cmdefprint
        fileprinthelper=vg_cm_thresholdhelper
        filecheck=vg_cmdefcheck
        fileedit=vg_cmdefedit
        listsize=10

        fields=(
            field0=(
                name=asset
                text="Asset Name"
                type=text
                info="the full name of an asset or a regular expression describing multiple assets: eg: n01234.* - common regular expressions meta characters allowed: $ ^ [ ] .*"
                helper=vg_cmdefvaluehelper
                search=yes
                size=60
            )
            field1=(
                name=sname
                text="Statistic Name"
                type=textro
                info="the name of the statistic to change the threshold on"
                helper=vg_cmdefvaluehelper
                search=yes
                size=60
            )
            field2=(
                name=sstatus
                text="Statistic Status"
                type=choice
                defval="keep|Active;drop|Disabled;noalarm|No Alarm"
                info="enable or disable the collection of this statistic"
                search=yes
            )
            field3=(
                name=svalue
                text="Statistic Threshold"
                type=textro
                info="the threshold parameters"
                helper=vg_cmdefvaluehelper
                search=yes
                size=60
            )
            field4=(
                name=account
                text="User account"
                type=textro
                info="the website account that established this threshold rule"
                search=yes
                size=60
            )
        )
        bulkinsertfield=(
            name=insertfile
            text="Bulk Insert File"
            info="a file with records to insert to the main file - must be in the same format as the main file"
            format="asset|metric|mode|alarmspec|account"
            example="labems01|fs_used|keep|>=95%:2:1/1:1/3600:CLEAR:5:1/1|swiftadmin"
        )
    )

    passwd=(
        name=passwd
        summary="The Password File"
        description="This file contains all the web site ids"

        hidden=yes

        group="vg_att_admin"

        location=$VGCM_PASSWDFILE
        filefuns=
        fileprint=
        filecheck=
        fileedit=
        listsize=20

        fields=(
            field0=(
                name=all
                text="all"
                type=text
                info="all"
                size=60
            )
        )
    )

    group=(
        name=group
        summary="The Group File"
        description="This file contains all the web site groups"

        hidden=yes

        group="vg_att_admin"

        location=$VGCM_GROUPFILE
        filefuns=
        fileprint=
        filecheck=
        fileedit=
        listsize=20

        fields=(
            field0=(
                name=all
                text="all"
                type=text
                info="all"
                size=60
            )
        )
    )

    account=(
        name=account
        summary="The Account File"
        description="This file contains all the web site account information"

        hidden=yes

        account="vg_att_admin"

        location=$VGCM_ACCOUNTFILE
        filefuns=
        fileprint=
        filecheck=
        fileedit=vg_cmaccountedit
        listsize=20

        fields=(
            field0=(
                name=all
                text="all"
                type=text
                info="all"
                size=60
            )
        )
    )

    preferences=(
        name=preferences

        summary="The Preferences Files"
        description="These files contain the user preferences"

        group="vg_*"
        accessmode=byid
        admingroup="vg_att_admin"

        location=$VGCM_PREFERENCESDIR
        locationmode=dir
        locationre='*.sh'
        filefuns=vg_cm_preferences
        filelist=vg_cmdeflist
        fileprint=vg_cmdefprint
        filecheck=vg_cmdefcheck
        fileedit=vg_cmdefedit
        listsize=10

        fields=(
            field0=(
                name=group
                text="Preference group"
                type=text
                info="the preference group"
                search=yes
                size=60
            )
            field1=(
                name=key
                text="Preference key"
                type=text
                info="the preference key"
                search=yes
                size=60
            )
            field2=(
                name=value
                text="Preference value"
                type=text
                info="the preference value"
                search=yes
                size=60
            )
            field3=(
                name=account
                text="User account"
                type=textro
                info="the website account that established this preference"
                search=yes
                size=60
            )
        )
    )


)
