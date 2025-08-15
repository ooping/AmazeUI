# 代码格式规范

## 1. 空格使用规范

### 1.1 运算符空格
- **所有二元运算符前后必须加空格**
  ```cpp
  // 正确
  int result = a + b * c;
  bool flag = (x == y) && (z != w);
  ptr = &variable;
  value = *ptr;
  
  // 错误
  int result=a+b*c;
  bool flag=(x==y)&&(z!=w);
  ```

### 1.2 赋值运算符空格
- **赋值运算符前后必须加空格**
  ```cpp
  // 正确
  x = 10;
  y += 5;
  z -= 3;
  
  // 错误
  x=10;
  y+=5;
  z-=3;
  ```

### 1.3 比较运算符空格
- **比较运算符前后必须加空格**
  ```cpp
  // 正确
  if (a > b) { }
  if (x <= y) { }
  if (z != 0) { }
  
  // 错误
  if (a>b) { }
  if (x<=y) { }
  if (z!=0) { }
  ```

## 2. 括号使用规范

### 2.1 表达式可读性
- **复杂表达式使用括号增加可读性**
  ```cpp
  // 正确
  result = (a + b) * (c - d);
  if ((x > 0) && (y < 100)) { }
  
  // 可接受但建议加括号
  result = a + b * c - d;
  if (x > 0 && y < 100) { }
  ```

### 2.2 函数调用括号
- **函数名与左括号之间不加空格**
  ```cpp
  // 正确
  function(param1, param2);
  obj.method(arg);
  
  // 错误
  function (param1, param2);
  obj.method (arg);
  ```

### 2.3 控制语句括号
- **控制语句关键字与左括号之间加一个空格**
  ```cpp
  // 正确
  if (condition) { }
  for (int i = 0; i < n; ++i) { }
  while (flag) { }
  
  // 错误
  if(condition) { }
  for(int i = 0; i < n; ++i) { }
  while(flag) { }
  ```

## 3. 大括号使用规范

### 3.1 大括号位置
- **左大括号与语句在同一行，前加一个空格**
  ```cpp
  // 正确
  if (condition) {
      // code
  }
  
  void function() {
      // code
  }
  
  // 错误
  if (condition)
  {
      // code
  }
  ```

### 3.2 else语句格式
- **else关键字与前面的右大括号在同一行，中间加一个空格**
  ```cpp
  // 正确
  if (condition) {
      statement1;
  } else {
      statement2;
  }
  
  if (condition) {
      statement1;
  } else if (condition2) {
      statement2;
  } else {
      statement3;
  }
  
  // 错误
  if (condition) {
      statement1;
  }
  else {
      statement2;
  }
  ```

### 3.3 单语句控制结构
- **即使只有一条语句，也建议使用大括号**
  ```cpp
  // 推荐
  if (condition) {
      statement;
  }
  
  // 可接受
  if (condition)
      statement;
  ```

## 4. 缩进与对齐规范

### 4.1 缩进
- **使用Tab字符进行缩进**
- **每级缩进一个Tab**
  ```cpp
  void function() {
  	if (condition) {
  		statement1;
  		statement2;
  	}
  }
  ```

### 4.2 参数对齐
- **函数参数过长时，每个参数单独一行并对齐**
  ```cpp
  // 正确
  function(param1,
           param2,
           param3);
  
  // 或者
  function(
      param1,
      param2,
      param3
  );
  ```

## 5. 变量声明规范

### 5.1 变量初始化
- **变量声明时的等号前后加空格**
  ```cpp
  // 正确
  int x = 0;
  bool flag = true;
  char buf[256] = {0};
  
  // 错误
  int x=0;
  bool flag=true;
  char buf[256]={0};
  ```

### 5.2 数组声明
- **数组大小与方括号之间不加空格**
  ```cpp
  // 正确
  int array[10];
  char buffer[256];
  
  // 错误
  int array[ 10 ];
  char buffer[ 256 ];
  ```

## 6. 循环语句规范

### 6.1 for循环格式
- **分号后加空格，运算符前后加空格**
  ```cpp
  // 正确
  for (int i = 0; i < n; ++i) {
      // code
  }
  
  for (size_t i = 0; i < list.size(); ++i) {
      // code
  }
  
  // 错误
  for (int i=0;i<n;++i) {
      // code
  }
  ```

### 6.2 while循环格式
- **条件表达式前后加空格**
  ```cpp
  // 正确
  while (condition) {
      // code
  }
  
  // 错误
  while(condition) {
      // code
  }
  ```

## 7. 函数定义规范

### 7.1 函数签名
- **参数列表中逗号后加空格**
  ```cpp
  // 正确
  bool function(int param1, char* param2, double param3);
  
  // 错误
  bool function(int param1,char* param2,double param3);
  ```

### 7.2 返回类型
- **返回类型与函数名之间加空格**
  ```cpp
  // 正确
  bool ProtoSerial::ComSend(const char *pBuf, int bufLen);
  
  // 错误
  bool ProtoSerial::ComSend(const char *pBuf, int bufLen);
  ```

## 8. 指针和引用规范

### 8.1 指针声明
- **星号紧贴类型名或变量名**
  ```cpp
  // 推荐
  char* buffer;
  int* ptr;
  
  // 可接受
  char *buffer;
  int *ptr;
  
  // 错误
  char * buffer;
  int * ptr;
  ```

### 8.2 取地址和解引用
- **取地址符和解引用符紧贴变量名**
  ```cpp
  // 正确
  ptr = &variable;
  value = *ptr;
  
  // 错误
  ptr = & variable;
  value = * ptr;
  ```

## 9. 特殊情况处理

### 9.1 条件表达式
- **复杂条件使用括号分组**
  ```cpp
  // 正确
  if ((a > b) && (c < d) || (e == f)) {
      // code
  }
  
  // 更清晰
  if (((a > b) && (c < d)) || (e == f)) {
      // code
  }
  ```

### 9.2 位移运算
- **位移运算符前后加空格**
  ```cpp
  // 正确
  mode = buf[0] << 24;
  mode += buf[1] << 16;
  
  // 错误
  mode = buf[0]<<24;
  mode += buf[1]<<16;
  ```

## 10. 注意事项

1. **不要将正确的格式改为错误格式**
2. **只调整格式，不改变代码逻辑**
3. **保持现有正确格式不变**
4. **统一整个项目的格式风格**
5. **特别注意运算符空格的一致性**
