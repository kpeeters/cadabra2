import codeop

str1="""for i in range(1000):
   if i!=0:
      sleep(0.001)
   out=display(i+1, cell_id=out)"""

str2="""for i in range(1000):
   if i!=0:
      sleep(0.001)
   out=display(i+1, cell_id=out)
end=time()"""

print(''.join(str2.rsplit('\n', 1)[:-1]))

res=codeop.compile_command(str1)
print("1 ok", res)
res=codeop.compile_command(str2)
print("2 ok", res)
