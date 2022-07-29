#!/bin/bash

# Figure out how to show files in layers / overlays / images and containers?

toplevels=""
widest_size=0
tag_header="REPOSITORY:TAG:NAME"
tag_header_us="-------------------"
widest_tag=${#tag_header}
state_width=10

# children[parent image id]="list of child image ids"
typeset -A children

# tags[image id]="list of tags for that image"
typeset -A tags

# sizes[image id]="the size of that image"
typeset -A sizes

# relative_sizes[image id]="size of this image relative to its parent"
typeset -A relative_sizes

# containers[image id]="list of container ids"
typeset -A containers

# container_names[container id]=name or command
typeset -A container_names

# container_states[container id]=state
typeset -A container_states

# iterate over all images, even untagged; all we need is the Id.
for id in $( docker images -a --format "{{.ID}}" )
do
    # the quotes are necessary to ensure empty strings and strings
    # with spaces in them stay in the same positional arg location.
    # oh, and Cmd must be last because $*.

    set -- $( docker inspect $id --format \
                     '{{.Id}} "{{.Parent}}" {{.Size}}
                     "{{.RepoTags}}" {{.ContainerConfig.Cmd}}' )

    # use 'sed' to grab the first 12 chars of the sha
    id=$(
        echo $1 |
            sed -e 's,sha256:\(............\).*,\1,'
      )
    shift

    # also use sed to strip the quotes as they're no longer needed.
    parent=$(
        echo $1 | \
            sed -e 's,\"sha256:\(............\).*,\1,' \
                -e 's,",,g'
          )
    shift

    size="$1"
    shift

    # repotags come out as "[json array]" so strip
    # the quotes and brackets.
    tag=$(
        echo $1 |
            sed -e 's,\[,,' \
                -e 's,\],,' \
                -e 's,",,g'
        )
    shift

    cmd=$(
        # commands come out kind of messy: json arrays,
        #   "# (nop) "  "  /bin/sh -c " and the like.
        # also commands can be long, arbitrarily cut to
        # a specific width.
        echo $* |
            sed -e 's,\[,,' -e 's,\],,' -e 's,/bin/sh -c ,,' \
                -e 's,(nop),,' -e 's,^#,,' -e 's,^ *,,' |
            cut -c1-40
        )

    if [[ "x$tag" == x ]] ; then

        # if this image has no tag, stick the CMD in there.
        tag=" :$cmd"

    fi

    sizes[$id]="$size"
    relative_sizes[$id]="$size"  # children get this overwritten.
    tags[$id]="$tag"

    # for pretty printing later, we'd like to know the
    # maximum column size needed.
    (( ${#size} > $widest_size )) && widest_size=${#size}
    (( ${#tag}  > $widest_tag  )) && widest_tag=${#tag}

    # collect the information required to build a tree.
    if [[ "x${parent}" == x ]] ; then

        # images with no parent are the top level
        toplevels="$toplevels $id"

    else

        # use the parent link to link parents to
        # children, but in reverse-- we will need to
        # follow child-lists to do indenting and grouping
        # properly.
        if [[ "x${children[$parent]}" == x ]] ; then

            # first child of this parent.
            children[$parent]=$id

        else

            # additional child to this parent.
            children[$parent]="${children[$parent]} $id"

        fi
    fi
done

# iterate over all containers, all we need is the id.
for id in $( docker ps -a --format='{{.ID}}' )
do
    set -- $( docker inspect $id --format \
                     '{{.ID}} {{.Image}} "{{.Name}}" {{.State.Status}}' )

    # use 'sed' to grab the first 12 chars of the sha
    id=$(
        echo $1 |
            sed -e 's,\(............\).*,\1,'
      )
    shift

    # also use sed to strip the quotes as they're no longer needed.
    image=$(
        echo $1 | \
            sed -e 's,sha256:\(............\).*,\1,' \
                -e 's,",,g'
          )
    shift

    name=$(
        echo $1 | \
            sed -e 's,",,g' -e s,^/,,
        )
    shift

    state="$1"
    shift

    if [[ x"${containers[$image]}" == "x" ]] ; then
        containers[$image]=$id
    else
        containers[$image]="${containers[$image]} $id"
    fi

    (( ${#name}  > $widest_tag  )) && widest_tag=${#name}

    container_names[$id]="$name"
    container_states[$id]="$state"

done

print_image_list() {
    local ids="$1"
    local indent="$2"
    local parentsize="$3"

    for id in $ids ; do
        local size=${sizes[$id]}
        print_image $id "$indent" "$size" "$parentsize"
        print_image_list "${children[$id]}"  "  $indent" "$size"
    done
}

# the widest size might just be relative, add one for the "+"
(( widest_size++ ))

widest_id=0

print_image() {
    local id="$1"
    local idind="$2$1"
    local size="$3"
    local parentsize="$4"
    local relsize
    local sizeprefix=""

    (( relsize = size - parentsize ))
    (( parentsize != 0 )) && sizeprefix="+"

    if [[ $doprint == 0 ]] ; then

        width=${#idind}

        # if this image has containers, we need 2 more chars
        [[ "${containers[$id]}" != "" ]] && (( width += 2 ))

        # first time thru, calculate widest id column.
        (( $width > $widest_id )) && widest_id=${width}

    else

        # second time thru, actually print it.
        printf \
"%-${widest_id}s %${sizeprefix}${widest_size}d %${widest_size}d %-${widest_tag}s %-${state_width}s\n" \
"$idind"         $relsize                      $size            "${tags[$id]}"   ""

        for cont in ${containers[$id]} ; do
            print_container "$cont" "C $indent"
        done

    fi
}

print_container() {
    local container="$1"
    local idind="$2$1"
    local name="${container_names[$container]}"
    local state="${container_states[$container]}"

    printf \
"%-${widest_id}s %${widest_size}s %${widest_size}s %-${widest_tag}s %-${state_width}s\n" \
"$idind"         ""               ""               "$name"           "$state"
}

# print the list twice, once with doprint=0,
# which simply calculates the widest identifier plus indent,
# once with doprint=1 to make pretty columns.
doprint=0
print_image_list "$toplevels" "" 0

# now that we know the column widths, print the column headers.
printf \
"%-${widest_id}s %${widest_size}s %${widest_size}s %-${widest_tag}s %-${state_width}s\n" \
"IMAGE ID"       "REL-SIZE"       "SIZE"           "$tag_header"    "STATE"
printf \
"%-${widest_id}s %${widest_size}s %${widest_size}s %-${widest_tag}s %-${state_width}s\n" \
"--------"       "--------"       "----"           "$tag_header_us" "-----"

# and print the data.
doprint=1
print_image_list "$toplevels" "" 0

exit 0
