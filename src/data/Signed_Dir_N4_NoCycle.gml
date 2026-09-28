graph [
  directed 1

  node [ id 1 label "1" ]
  node [ id 2 label "2" ]
  node [ id 3 label "3" ]
  node [ id 4 label "4" ]

  edge [
    source 1
    target 2
    value 3
  ]
  edge [
    source 2
    target 3
    value -2
  ]
  edge [
    source 3
    target 4
    value 4
  ]
  edge [
    source 1
    target 4
    value 10
  ]
]
